#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
���������쳲��������У�Fibonacci sequence�����ֳƻƽ�ָ����У�ָ��������һ�����У�0��1��1��2��3��5��8��13��21��34��������
"""
def fib(n):
    if n < 2:
        return n
    else:
        return fib(n-1) + fib(n-2)
for i in range(0,20):
    print fib(i)
    
"""
#��ķ�����
 
def fib(n):
    a,b = 1,1
    for i in range(n-1):
        a,b = b,a+b
    return a
 
# ����˵�10��쳲���������
print fib(10)
"""