#!/usr/bin/env python
# -*- coding: utf-8 -*-

from selenium import webdriver
import time

driver = webdriver.Chrome()
time.sleep(2)

driver.set_window_size(240,320)
driver.get('http://www.3g.qq.com')

time.sleep(5)
print 'close browser'

driver.quit()