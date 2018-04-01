#!/usr/bin/env python
# -*- coding: utf-8 -*- 

from selenium import webdriver
from selenium.webdriver.common.desired_capabilites import DesiredCapabilities

driver = webdriver.Remote(command_executor='http://127.0.0.1:4444/wd/hub',desired_capabilites=DesiredCapabilities.CHROME)
driver = webdriver.Remote(command_executor='http://127.0.0.1:4444/wd/hub',desired_capabilites=DesiredCapabilities.OPERA)
driver = webdriver.Remote(command_executor='http://127.0.0.1:4444/wd/hub',desired_capabilites=DesiredCapabilities.HTMLUNITWITHJS)
driver = webdriver.Remote(command_executor='http://127.0.0.1:4444/wd/hub',desired_capabilites=DesiredCapabilities={'browserName':'htmlunit','version':'2','javascriptEnabled':True})