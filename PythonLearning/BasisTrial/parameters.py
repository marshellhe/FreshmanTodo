#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
def power(x,n):
    while n >= 2:
        x = x*x
        n = n -1
    return x
print(power(10,0))
"""
"""
def power(x,n=2):
    s = 1
    while n > 0:
        n = n - 1
        s = s * x
    return s
"""
"""
def add_end(L=[]):
    L.append("END")
    return L
"""
def add_end(L=None):
    L = []
    L.append('END')
    return L