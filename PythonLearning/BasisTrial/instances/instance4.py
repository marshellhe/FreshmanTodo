#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""��Ŀ������ĳ��ĳ��ĳ�գ��ж���һ������һ��ĵڼ��죿"""
year = int(raw_input("��������ݣ�"))
month = int(raw_input("�������·�:"))
date = int(raw_input("�������գ�"))
a = [0,31,59,90,120,151,181,212,253,283,314,334,365]
if month == 1:
    r = date
elif year%4 == 0 and year%100 != 0 or year%400 == 0 and month > 2:
    r = a[month-1] + date + 1
else:
    r = a[month-1] + date
print "����һ��ĵ�%s��"%r
        