
Main watchdog registers:
- watchdog timer counter register 	 (WDT_WCRR)
- watchdog timer load register 		 (WDT_WLDR) 
- watchdog timer trigger register 	 (WDT_WTGR)
- watchdog timer start/stop register (WDT_WSPR)

==============================================
Triggering a Timer Reload:
- The specific reload sequence is performed whenever
the written value on the WDT_WTGR register differs
from its previous value.

==============================================
Start/Stop Sequence for Watchdog Timers (Using the WDT_WSPR Register)
To disable the timer, follow this sequence:
1. Write XXXX AAAAh in WDT_WSPR.
2. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.
3. Write XXXX 5555h in WDT_WSPR.
4. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.
To enable the timer, follow this sequence:
1. Write XXXX BBBBh in WDT_WSPR.
2. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.
3. Write XXXX 4444h in WDT_WSPR.
4. Poll for posted write to complete using WDT_WWPS.W_PEND_WSPR.