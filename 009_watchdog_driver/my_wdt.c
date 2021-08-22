#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/platform_data/omap-wd-timer.h>

#define WDT_WIDR		0x00 
#define WDT_WDSC		0x10
#define WDT_WDST		0x14
#define WDT_WCLR		0x24 /*Watchdog Control Register*/
#define WDT_WCRR		0x28 /*Watchdog Counter Register*/
#define WDT_WLDR		0x2c /*Watchdog Load Register*/
#define WDT_WTGR		0x30 /*Watchdog Trigger Register*/
#define WDT_WWPS		0x34 /*Watchdog Write Posting Bits Register*/
#define WDT_WSPR		0x48 /*Watchdog Start/Stop Register*/
#define WDT_WDLY		0x44 /*Watchdog Delay Configuration Register*/


#define PTV			0	/* prescale */
#define GET_WLDR_VAL(secs)	(0xffffffff - ((secs) * (32768/(1<<PTV))) + 1)
#define GET_WCCR_SECS(val)	((0xffffffff - (val) + 1) / (32768/(1<<PTV)))

#define TIMER_MARGIN_MAX	(24 * 60 * 60)	/* 1 day */
#define TIMER_MARGIN_DEFAULT	60	/* 60 secs */
#define TIMER_MARGIN_MIN	1


struct wdt_dev_data {
	struct watchdog_device wdog;
	struct mutex	my_lock;
	void __iomem    *base;          /* physical */
	struct device   *dev;
	bool		omap_wdt_users;
	int		wdt_trgr_pattern;
};

static void demo_wdt_enable(struct wdt_dev_data *wdev)
{
/*
1. Write XXXX BBBBh in WDT_WSPR.
2. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.
3. Write XXXX 4444h in WDT_WSPR.
4. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.
*/
	void __iomem *base = wdev->base;

	writel_relaxed(0xBBBB, base + WDT_WSPR);
	while ((readl_relaxed(base + WDT_WWPS)) & 0x10)
		cpu_relax();

	writel_relaxed(0x4444, base + WDT_WSPR);
	while ((readl_relaxed(base + WDT_WWPS)) & 0x10)
		cpu_relax();
}

static void demo_wdt_disable(struct wdt_dev_data *wdev)
{
/*
1. Write XXXX BBBBh in WDT_WSPR.
2. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.
3. Write XXXX 4444h in WDT_WSPR.
4. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR
*/
	void __iomem *base = wdev->base;
	
	writel_relaxed(0xAAAA, base + WDT_WSPR);	
	while (readl_relaxed(base + WDT_WWPS) & 0x10)
		cpu_relax();

	writel_relaxed(0x5555, base + WDT_WSPR);	
	while (readl_relaxed(base + WDT_WWPS) & 0x10)
		cpu_relax();
}

static void demo_wdt_reload(struct wdt_dev_data *wdev)
{
	void __iomem    *base = wdev->base;

	/* wait for posted write to complete */
	while ((readl_relaxed(base + WDT_WWPS)) & 0x08)
		cpu_relax();

	wdev->wdt_trgr_pattern = ~wdev->wdt_trgr_pattern;
	writel_relaxed(wdev->wdt_trgr_pattern, (base + WDT_WTGR));

	/* wait for posted write to complete */
	while ((readl_relaxed(base + WDT_WWPS)) & 0x08)
		cpu_relax();
	/* reloaded WCRR from WLDR */
}

static void demo_wdt_set_timer(struct wdt_dev_data *wdev,
				   unsigned int timeout)
{
	u32 pre_margin = GET_WLDR_VAL(timeout);
	void __iomem *base = wdev->base;

	/* just count up at 32 KHz */
	while (readl_relaxed(base + WDT_WWPS) & 0x04)
		cpu_relax();

	writel_relaxed(pre_margin, base + WDT_WLDR);
	while (readl_relaxed(base + WDT_WWPS) & 0x04)
		cpu_relax();
}

static int demo_wdt_start(struct watchdog_device *wdog)
{
	struct wdt_dev_data *wdev = container_of(wdog, struct wdt_dev_data, wdog);
	void __iomem *base = wdev->base;
	
	mutex_lock(&wdev->my_lock);
	
	wdev->omap_wdt_users = true;
	pm_runtime_get_sync(wdev->dev);
	
	/*1. Disable the watchdog timer.*/
	demo_wdt_disable(wdev);
	
	/*2. Enable prescaler with Clock Divider (PS) = 1*/
	while ((readl_relaxed(base + WDT_WWPS)) & 0x01)
		cpu_relax();
	writel_relaxed((1 << 5) | (PTV << 2), wdev->base + WDT_WCLR);
	while ((readl_relaxed(base + WDT_WWPS)) & 0x01)
		cpu_relax();
	
	/*3. Load delay configuration value.*/
	
	/*4. Load timer counter value.*/
	demo_wdt_set_timer(wdev, wdog->timeout);
	demo_wdt_reload(wdev); /* trigger loading of new timeout value */
	
	
	/*5. Enable the watchdog timer.*/
	demo_wdt_enable(wdev);
	
	
	mutex_unlock(&wdev->my_lock);
	return 0;
}

static int demo_wdt_stop(struct watchdog_device *wdog)
{
	struct wdt_dev_data *wdev = container_of(wdog, struct wdt_dev_data, wdog);

	mutex_lock(&wdev->my_lock);
	demo_wdt_disable(wdev);
	pm_runtime_put_sync(wdev->dev);
	wdev->omap_wdt_users = false;
	mutex_unlock(&wdev->my_lock);
	return 0;
}

static int demo_wdt_ping(struct watchdog_device *wdog)
{
	struct wdt_dev_data *wdev = container_of(wdog, struct wdt_dev_data, wdog);
	mutex_lock(&wdev->my_lock);
	demo_wdt_reload(wdev);
	mutex_unlock(&wdev->my_lock);

	return 0;
}

static int demo_wdt_set_timeout(struct watchdog_device *wdog,
				unsigned int timeout)
{
	struct wdt_dev_data *wdev = container_of(wdog, struct wdt_dev_data, wdog);
	mutex_lock(&wdev->my_lock);
	demo_wdt_disable(wdev);
	demo_wdt_set_timer(wdev, timeout);
	demo_wdt_enable(wdev);
	demo_wdt_reload(wdev);
	wdog->timeout = timeout;
	mutex_unlock(&wdev->my_lock);
	return 0;
}

static unsigned int demo_wdt_get_timeleft(struct watchdog_device *wdog)
{
	struct wdt_dev_data *wdev = container_of(wdog, struct wdt_dev_data, wdog);
	void __iomem *base = wdev->base;
	u32 value;

	value = readl_relaxed(base + WDT_WCRR);
	return GET_WCCR_SECS(value);
}



static const struct watchdog_ops demo_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= demo_wdt_start,
	.stop		= demo_wdt_stop,
	.ping		= demo_wdt_ping,
	.set_timeout	= demo_wdt_set_timeout,
	.get_timeleft	= demo_wdt_get_timeleft,
};

static int demo_wdt_probe(struct platform_device *pdev)
{
	struct wdt_dev_data *wdev;
	int ret;
	pr_emerg("%s, %d", __func__, __LINE__);
	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;
	
	wdev->omap_wdt_users	= false;
	wdev->dev		= &pdev->dev;
	wdev->wdt_trgr_pattern	= 0x0310; /*Write anything to the WDT_WTGR */
	mutex_init(&wdev->my_lock);
	
	wdev->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(wdev->base))
		return PTR_ERR(wdev->base);	
	
	
	wdev->wdog.ops = &demo_wdt_ops;
	wdev->wdog.min_timeout = TIMER_MARGIN_MIN;
	wdev->wdog.max_timeout = TIMER_MARGIN_MAX;
	wdev->wdog.timeout = TIMER_MARGIN_DEFAULT;
	wdev->wdog.parent = &pdev->dev;
	
	/*Set driver data for wdev struct*/
	platform_set_drvdata(pdev, wdev);
	
	pm_runtime_enable(wdev->dev);
	pm_runtime_get_sync(wdev->dev);
	
	ret = watchdog_register_device(&wdev->wdog);
	if (ret){
		return ret;
	}
	
	return 0;
	
}

static void demo_wdt_shutdown(struct platform_device *pdev)
{
	struct wdt_dev_data *wdev = platform_get_drvdata(pdev);

	mutex_lock(&wdev->my_lock);
	if (wdev->omap_wdt_users) {
		demo_wdt_disable(wdev);
		pm_runtime_put_sync(wdev->dev);
	}
	mutex_unlock(&wdev->my_lock);

}

static int demo_wdt_remove(struct platform_device *pdev)
{
	struct wdt_dev_data *wdev = platform_get_drvdata(pdev);
	pr_emerg("%s, %d", __func__, __LINE__);
	watchdog_unregister_device(&wdev->wdog);
	return 0;
}

static const struct of_device_id demo_wdt_of_match[] = {
	{ .compatible = "ti,my-wdt", },
	{},
};
MODULE_DEVICE_TABLE(of, demo_wdt_of_match);

static struct platform_driver demo_wdt_driver = {
	.probe		= demo_wdt_probe,
	.remove		= demo_wdt_remove,
	.shutdown	= demo_wdt_shutdown,
	.driver		= {
		.name   = "demo_wdt",
		.of_match_table = demo_wdt_of_match,
	},
};

module_platform_driver(demo_wdt_driver);

MODULE_AUTHOR("Hoang Nguyen");
MODULE_LICENSE("GPL");


