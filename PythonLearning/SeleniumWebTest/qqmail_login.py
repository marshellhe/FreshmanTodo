#!/usr/bin/env python
# -*- coding: utf-8 -*-
from selenium import webdriver
import sys
import time

reload(sys)
sys.setdefaultencoding('utf8')

driver = webdriver.Chrome()
#newһ�������ʵ�����������
driver.get("https://mail.qq.com/cgi-bin/loginpage")
#get�������������URL����
assert "��¼QQ����".decode('gbk').encode('utf-8') in driver.title

"""
driver.find_element_by_xpath("//*[@id="u"]").clear()
driver.find_element_by_xpath("//*[@id="u"]").send_keys('jminhe@qq.com')
driver.find_element_by_xpath('//*[@id="p"]').send_keys('Q37#ARsym')
driver.find_element_by_xpath('//*[@id="pp"]').send_keys('jm#R2ver.com')
"""
#find_element_by_*Ԫ�ض�λ����
#assert "QQ����" in driver.title

time.sleep(2)
print "already done or game over"

driver.quit()
#�ر������tabҳ��Ҳ������exit()���棬�����ǹر����������


