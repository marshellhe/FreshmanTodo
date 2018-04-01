#!/usr/bin/env python
# -*- coding:utf-8 -*-

test_list = [3,5,7,8,43,23,55,6,7,8]
def converse(list):
    temp = []
    for i in range(0,len(list)):
         temp.append(list[len(list)-i-1])
#        temp[i] = list[len(list)-i-1]   #temp一开始是空数组，对空数组赋值会产生越界
    return temp
print converse(test_list)