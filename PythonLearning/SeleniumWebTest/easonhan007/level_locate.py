#!/usr/bin/env python
# -*- coding: utf-8 -*-

from selenium import webdriver
from selenium.webdriver.support.ui import WebDriverWait
import time
import os

dr = webdriver.Chrome()
file_path = 'file:////' + os.path.abspath('level_locate.html')
print file_path

dr.get(file_path)


dr.find_element_by_link_text('Link1').click()

WebDriverWait(dr,15).until(lambda the_driver:the_driver.find_element_by_class_name('dropdown-menu').is_displayed())
#WebDriverWait主要有两个方法until和until_not
#上面语句意思是15秒内等待该元素被定位到
menu = dr.find_element_by_class_name('dropdown-menu').find_element_by_link_text('Another action')

webdriver.ActionChains(dr).move_to_element(menu).perform()

time.sleep(2)

dr.quit()

"""
程序原来是用WebDriverWait等待时间10s，结果发现经常因为请求https://ajax.googleapis.com/ajax/libs/jquery/1.9.1/jquery.min.js等待超时而报错
后改成15s可以执行通过了
"""