#!/usr/bin/env python
# -*- coding: utf-8 -*-
def my_abs(x):
    if not isinstance(x,(int,float)):
        raise TypeError('bad operand type')
    if x >= 0:
        return x
    else:
        return -x
a = raw_input(u'你好，请输入一个数字:'.encode('gbk'))
print u'%s 的绝对值是%s'%(a,my_abs(int(a)))
#print my_abs(int(a))