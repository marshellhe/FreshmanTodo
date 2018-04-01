#!/usr/bin/env python
# -*- coding: utf-8 -*-
from selenium import webdriver
import sys
import time

reload(sys)
sys.setdefaultencoding('utf8')

driver = webdriver.Chrome()
#new一个浏览器实例，打开浏览器
driver.get("https://mail.qq.com/cgi-bin/loginpage")
#get方法导向给定的URL链接
assert "登录QQ邮箱".decode('gbk').encode('utf-8') in driver.title

"""
driver.find_element_by_xpath("//*[@id="u"]").clear()
driver.find_element_by_xpath("//*[@id="u"]").send_keys('jminhe@qq.com')
driver.find_element_by_xpath('//*[@id="p"]').send_keys('Q37#ARsym')
driver.find_element_by_xpath('//*[@id="pp"]').send_keys('jm#R2ver.com')
"""
#find_element_by_*元素定位方法
#assert "QQ邮箱" in driver.title

time.sleep(2)
print "already done or game over"

driver.quit()
#关闭浏览器tab页，也可以用exit()代替，作用是关闭整个浏览器


