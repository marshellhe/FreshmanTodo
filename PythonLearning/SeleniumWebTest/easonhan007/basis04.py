#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""求100以内质数

for num in range(1,101):
    for i in range(2,num):
        if (num % i) == 0:
            break
    else:
        print num  

"""
"""判断数值
写个程序，随机生成一个数，提示用户输入一个数字，如果用户输入的比这个大，提示用户大于，否则提示用户小于，直到用户猜到这个数为止；"""
"""
import random
a = random.randint(0,5)
b = int(raw_input("请输入一个整数:")) 
while not b == a:
    if b < a:
        print '比您输入的整数%s更大'%b
        b = int(raw_input("请继续输入一个整数:")) 
    else:
        print '比您输入的整数%s更小'%b
        b = int(raw_input("请继续输入一个整数:")) 
if b == a:
    print '输入正确，恭喜您！'
"""

for i in range(2017,2037):
    if (i % 4 == 0) and (i % 100 != 0) or (i % 100 == 0) and (i % 400 ==0):
        print '%s是闰年'%i