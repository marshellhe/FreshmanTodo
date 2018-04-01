#!/usr/bin/python
# -*- coding: UTF-8 -*-
x = int(raw_input("请输入第一个整数:"))
y = int(raw_input("请输入第二个整数:"))
z = int(raw_input("请输入第三个整数:"))
print "初始输入的3个整数为：%d,%d,%d"%(x,y,z)
"""
def Max(m,n):
    if m > n:
        return m
    else:
        return n
def Min(o,p):
    if o > p:
        return p
    else:
        return o
if z > Max(x,y):
    print "输入的3个整数按从大到小排序为：%d,%d,%d"%(z,Max(x,y),Min(x,y))
elif z <= Min(x,y):
    print "输入的3个整数按从大到小排序为：%d,%d,%d"%(Max(x,y),Min(x,y),z)
else:
    print "输入的3个整数按从大到小排序为：%d,%d,%d"%(Max(x,y),z,Min(x,y))
    """
l = []
for i in [x,y,z]:
    l.append(i)
l.sort()
l.reverse()
print l