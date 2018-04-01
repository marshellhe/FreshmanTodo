#-*-coding:utf-8-*-
import requests
from lxml import html

cookie = {}

raw_cookies = 'bid	QnYfuAJLbuM	/	.douban.com	2018/6/22 下午5:05:19 __utma	30149280.282132545.1498122323.1498122323.1498180506.2	/	.douban.com	2019/6/23 上午9:16:00 __utmz	30149280.1498122323.1.1.utmcsr=(direct)|utmccn=(direct)|utmcmd=(none)	/	.douban.com	2017/12/22 下午9:16:00 ap	1	/	.douban.com	2017/7/23 上午9:38:55 __utmc	30149280	/	.douban.com	End Of Session ps	y	/	.douban.com	2017/7/23 上午9:14:43 push_noty_num	0	/	.douban.com	2017/7/23 上午9:16:00 push_doumail_num	0	/	.douban.com	2017/7/23 上午9:16:00 __utmb	30149280.6.10.1498180506	/	.douban.com	2017/6/23 上午9:46:00 __utmv	30149280.16285	/	.douban.com	2019/6/23 上午9:16:00 as	"https://www.douban.com/people/sharplhl/"	/	.douban.com	2017/6/24 下午5:39:05'

for line in raw_cookies.split(';'):
    key,value = line.split("=",1)
    cookie[key] = value
    
page = requests.get('https://www.douban.com/people/sharplhl/',cookies = cookie)

tree = html.fromstring(page.text)

intro_raw = tree.xpath("id('line1')/span[533]")

intro =[]
for i in intro_raw:
    intro = i.encode('utf-8')

print intro



