#!/usr/bin/env python
# -*- coding: utf-8 -*-
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
#selenium.webdriver�ṩ����WebDriver��ִ�з���������֧��Firefox��Chrome��#Keys���ṩ������������м�
driver = webdriver.Chrome()
#newһ�������ʵ�����������
driver.get("https://www.python.org/")
#get�������������URL����
assert "Python" in driver.title
#ͨ��assert�����ж�ȷ����������ѱ����а����ؼ��ʡ�Python��
elem = driver.find_element_by_id('id-search-field')
#find_element_by_*Ԫ�ض�λ����
elem.click

elem.clear()
elem.send_keys("pycon")
elem.send_keys(Keys.RETURN)
#����������֮ǰ����գ������������ݲ��س�

assert "No results found." not in driver.page_source
#ͨ��assert����ȷ��������ȷ�����

driver.close()
#�ر������tabҳ��Ҳ������exit()���棬�����ǹر����������

#//*[@id="id-search-field"]


