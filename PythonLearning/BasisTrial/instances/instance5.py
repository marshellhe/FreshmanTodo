#!/usr/bin/python
# -*- coding: UTF-8 -*-
x = int(raw_input("�������һ������:"))
y = int(raw_input("������ڶ�������:"))
z = int(raw_input("���������������:"))
print "��ʼ�����3������Ϊ��%d,%d,%d"%(x,y,z)
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
    print "�����3���������Ӵ�С����Ϊ��%d,%d,%d"%(z,Max(x,y),Min(x,y))
elif z <= Min(x,y):
    print "�����3���������Ӵ�С����Ϊ��%d,%d,%d"%(Max(x,y),Min(x,y),z)
else:
    print "�����3���������Ӵ�С����Ϊ��%d,%d,%d"%(Max(x,y),z,Min(x,y))
    """
l = []
for i in [x,y,z]:
    l.append(i)
l.sort()
l.reverse()
print l