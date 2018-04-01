#!/usr/bin/env python
# -*- coding: utf-8 -*-

from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from time import sleep
import os

if 'HTTP_PROXY' in os.environ: del os.environ['HTTP_PROXY']

dr = webdriver.Chrome()
file_path = 'file:////' + os.path.abspath('button_group.html')

dr.get(file_path)

sleep(1)

#定位text是second的按钮
buttons = dr.find_element_by_class_name('btn-group').find_elements_by_class_name('btn')
for btn in buttons:
    if btn.text == 'second':
        print 'find the second button'
        
print dr.find_element_by_link_text('second').get_attribute('class')
sleep(1)
dr.quit()
"""
先找到button group的父div，class为btn-group的div，然后再找到下面所有的div(也就是button)，返回text是second的div
"""