#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
程序分析：斐波那契数列（Fibonacci sequence），又称黄金分割数列，指的是这样一个数列：0、1、1、2、3、5、8、13、21、34、……。
"""
def fib(n):
    if n < 2:
        return n
    else:
        return fib(n-1) + fib(n-2)
for i in range(0,20):
    print fib(i)
    
"""
#别的方法：
 
def fib(n):
    a,b = 1,1
    for i in range(n-1):
        a,b = b,a+b
    return a
 
# 输出了第10个斐波那契数列
print fib(10)
"""