import sys
import requests
import bs4
import argparse
from beautifultable import BeautifulTable

base_url = "https://www.thewatchpages.com/watches/all/?"

convert_size = {'XS': 370,
                'S': 368,
                'M': 367,
                'L': 366,
                'supersized': 369}
                      
class Swapping_Web:
    def __init__(self, argvs):
        self.argvs = argvs

    def gen_url(self):
        extra_url = ''
        if self.argvs.size:
            size = self.argvs.size
            extra_url += "&size={}".format(convert_size[size])
        if self.argvs.gender:
            gender = self.argvs.gender
            extra_url += "&gender={}".format(gender)
        if self.argvs.brand:
            brand = self.argvs.brand
            extra_url += "&brand={}".format(brand)

        return extra_url        

    def run(self):
        row_cnt = []
        page_number = 0
        num_found_watches = 0
        
        table = BeautifulTable()
        table.columns.header = ["Category ", "Watch model", "Price(USD)", "Link"]
        extra_url = self.gen_url()
        watches_url = [base_url + extra_url]
        
        result = requests.get(watches_url[0])
        soup = bs4.BeautifulSoup(result.text,"lxml")

        if(len(soup.select('.pagination')[0].find_all('a')) >= 2):
            page_number = int(soup.select('.pagination')[0].find_all('a')[len(soup.select('.pagination')[0].find_all('a'))-2].text)

        if page_number >= 2:
            for page in range(2, page_number+1, 1):
                next_url = base_url[:-1] + "page/{}".format(page) + base_url[-1:] + extra_url
                watches_url.append(next_url)

        print(watches_url)
        for url in watches_url:
            result = requests.get(url)
            soup = bs4.BeautifulSoup(result.text,"lxml")
            for i in range(len(soup.find_all('article'))):
                num_found_watches +=1
            
        
            for watch in soup.find_all('article'):
                category = watch.select('.block')[0].text
                model = watch.select('.block')[1].text
                price = watch.select('.watch-price')[0].text.strip()
                href = watch.a.get('href')
                
                table.rows.append([category, model, price, href])

        
        for i in range(num_found_watches):
                row_cnt.append('{}'.format(i))
        table.rows.header = row_cnt
        print("Your search has returned {} watches".format(num_found_watches))
        print(table)
            
    
def help_user():
    parser=argparse.ArgumentParser(
        description='''How to use the python script. ''')
    parser.add_argument('--size',choices=['XS', 'S', 'M', 'L', 'supersized'], type=str, help='Size of the Watch!')

    parser.add_argument('--gender',choices=['male', 'female'], type=str, help='Your gender!')

    parser.add_argument('--brand', type=str, help='Brand of the Watch!')
    if len(sys.argv)==1:
        parser.print_help(sys.stderr)
        sys.exit(1)
    
    return parser.parse_args()

if __name__ == "__main__":
    argvs = help_user()
    swapper = Swapping_Web(argvs)
    swapper.run()
    
    
    
    
