#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""题目：输入某年某月某日，判断这一天是这一年的第几天？"""
year = int(raw_input("请输入年份："))
month = int(raw_input("请输入月份:"))
date = int(raw_input("请输入日："))
a = [0,31,59,90,120,151,181,212,253,283,314,334,365]
if month == 1:
    r = date
elif year%4 == 0 and year%100 != 0 or year%400 == 0 and month > 2:
    r = a[month-1] + date + 1
else:
    r = a[month-1] + date
print "这是一年的第%s天"%r
        