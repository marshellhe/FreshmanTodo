#!/usr/bin/env python
# -*- coding: utf-8 -*-

from selenium import webdriver
from time import sleep
import os
if 'HTTP_PROXY' in os.environ:del os.environ['HTTP_PROXY']

dr = webdriver.Chrome()
file_path = 'file:////' + os.path.abspath('form.html')
print file_path

dr.get(file_path)

#by id
dr.find_element_by_id('inputEmail').click()
print dr.find_element_by_id('inputEmail').get_attribute('type')

#by name
dr.find_element_by_name('password').click()
print dr.find_element_by_name('password').get_attribute('placeholder')

#by tag_name
print dr.find_element_by_tag_name('form').get_attribute('class')


#by class_name
print dr.find_element_by_class_name('btn').get_attribute('type')
#dr.execute_script('$(arguments[0]).fadeOut().fadeIn()',e)


# by link text
print dr.find_element_by_link_text('register').get_attribute('id')
#dr.execute_script('$(arguments[0]).fadeOut().fadeIn()', link)

# by partial link text
print dr.find_element_by_partial_link_text('regi').get_attribute('name')

sleep(1)

# by css selector
print dr.find_element_by_css_selector('#inputEmail').get_attribute('name')

# by xpath
dr.find_element_by_xpath('/html/body/form/div[3]/div/label/input').click()

dr.quit()