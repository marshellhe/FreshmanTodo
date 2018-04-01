#!/usr/bin/env python
# -*- coding:utf-8 -*-

from selenium import webdriver
from selenium.webdriver.common.keys import Keys
import time

driver = webdriver.Chrome()
driver.get('http://www.zhaopin.com/')
loginname = driver.find_element_by_id('loginname')
password = driver.find_element_by_id('password')
login = driver.find_element_by_xpath('//*[@id="loginForm"]/div[3]/button')
loginname.clear()
password.clear()
"""
loginname.send_keys('jminheqq.com')
password.send_keys('566008hjm')
login.click()
assert u'手机号或邮箱不存在，请确认后重新输入！' in driver.page_source
"""

loginname.send_keys('jminhe@qq.com')
password.send_keys('566008hjm')
assert u'您输入的密码与账号不匹配' not in driver.page_source



time.sleep(2)

driver.quit()