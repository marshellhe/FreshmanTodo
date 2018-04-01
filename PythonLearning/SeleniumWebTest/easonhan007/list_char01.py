#!/usr/bin/env python
# -*- coding:utf-8 -*-

test_list = [3,5,7,8,43,23,55,6,7,8]
def max(list):
    temp = 0
    for i in list:
        if i >= temp:
            temp = i
    return temp
print max(test_list)