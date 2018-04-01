#!/usr/bin/env python
# -*- coding: utf-8 -*-
import unittest
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
"""
Selenium包自身不提供测试框架，可以使用Python内置的unittest模块，或是py.test和nose
unittest模块基于Java的Junit的Python内置模块(built-in module)，用
来组织test case的测试框架
"""

class PythonOrgSearch(unittest.TestCase):
#当前测试脚本类继承自TestCase，用于告诉unittest这是一个test case  
    def setUp(self):
        self.driver = webdriver.Chrome()
    #setUp 方法用于脚本类的初始化
    #testcase方法必须以‘test’开头
    def test_search_in_python_org(self):
        driver = self.driver
        driver.get("http://www.python.org")
        self.assertIn("Python",driver.title)
        driver.save_screenshot('screenshot.png')
        elem = driver.find_element_by_name("q")
        elem.clear()
        elem.send_keys("pycon")
        elem.send_keys(Keys.RETURN)
        assert "No results found." not in driver.page_source
        
    def tearDown(self):
        self.driver.close()
    #tearDown方法用于测试脚本的清理动作
if __name__ == "__main__":
    unittest.main()
#用于执行 当前test suite