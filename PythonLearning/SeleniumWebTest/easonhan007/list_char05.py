#!/usr/bin/env python
# -*- coding:utf-8 -*-

test_list = [3,5,7,8,43,23,55,6,7,8]
def count(list):
    return len(list)

def count1(list):
    temp = 0
    for i in list:
        list.pop()
        temp += 1
    return temp
    
print count(test_list)
print count1(test_list)
