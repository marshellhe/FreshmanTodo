#!/usr/bin/env python
# -*- coding: utf-8 -*-
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
#selenium.webdriver�ṩ����WebDriver��ִ�з���������֧��Firefox��Chrome��#Keys���ṩ������������м�
driver = webdriver.Chrome()
#newһ�������ʵ�����������
driver.get("https://deveco.huawei.com")
#get�������������URL����
assert "DevEco" in driver.title
#ͨ��assert�����ж�ȷ����������ѱ����а����ؼ��ʡ�Python��
elem_contact = driver.find_element_by_class_name('icon-contactus')
#find_element_by_*Ԫ�ض�λ����
elem_contact.click()

#driver.switch_to_alert().accept()

driver.find_element_by_id('problemDesc').send_keys("pycon")

driver.find_element_by_id('contactInfo').send_keys("test@autotest.com")

driver.find_element_by_id('certain').click()

print "already done or game over"

driver.close()
#�ر������tabҳ��Ҳ������exit()���棬�����ǹر����������




