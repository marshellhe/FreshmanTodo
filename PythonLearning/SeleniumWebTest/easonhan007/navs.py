#!/usr/bin/env python
# -*- coding: utf-8 -*-

from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from time import sleep
import os

if 'HTTP_PROXY' in os.environ: del os.environ['HTTP_PROXY']

dr = webdriver.Chrome()
file_path = 'file:////' + os.path.abspath('navs.html')

dr.get(file_path)

sleep(1)

#方法1：层级定位，先定位ul再定位li
dr.find_element_by_class_name('nav').find_element_by_link_text('About').click()

#方法2：直接定位link
dr.find_element_by_link_text('Home').click()

sleep(3)
dr.quit()
"""
先找到button group的父div，class为btn-group的div，然后再找到下面所有的div(也就是button)，返回text是second的div
"""