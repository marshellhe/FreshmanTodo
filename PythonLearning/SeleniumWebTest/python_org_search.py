#!/usr/bin/env python
# -*- coding: utf-8 -*-
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
#selenium.webdriver提供所有WebDriver的执行方法，包括支持Firefox、Chrome等#Keys类提供键盘输入的所有键
driver = webdriver.Chrome()
#new一个浏览器实例，打开浏览器
driver.get("https://www.python.org/")
#get方法导向给定的URL链接
assert "Python" in driver.title
#通过assert断言判断确定进入的网友标题中包含关键词“Python”
elem = driver.find_element_by_id('id-search-field')
#find_element_by_*元素定位方法
elem.click

elem.clear()
elem.send_keys("pycon")
elem.send_keys(Keys.RETURN)
#输入搜索框之前先清空，输入搜索内容并回车

assert "No results found." not in driver.page_source
#通过assert断言确定搜索有确定结果

driver.close()
#关闭浏览器tab页，也可以用exit()代替，作用是关闭整个浏览器

#//*[@id="id-search-field"]


