#!/usr/bin/env python
# -*- coding: utf-8 -*-
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
#selenium.webdriver提供所有WebDriver的执行方法，包括支持Firefox、Chrome等#Keys类提供键盘输入的所有键
driver = webdriver.Chrome()
#new一个浏览器实例，打开浏览器
driver.get("https://deveco.huawei.com")
#get方法导向给定的URL链接
assert "DevEco" in driver.title
#通过assert断言判断确定进入的网友标题中包含关键词“Python”
elem_contact = driver.find_element_by_class_name('icon-contactus')
#find_element_by_*元素定位方法
elem_contact.click()

#driver.switch_to_alert().accept()

driver.find_element_by_id('problemDesc').send_keys("pycon")

driver.find_element_by_id('contactInfo').send_keys("test@autotest.com")

driver.find_element_by_id('certain').click()

print "already done or game over"

driver.close()
#关闭浏览器tab页，也可以用exit()代替，作用是关闭整个浏览器




