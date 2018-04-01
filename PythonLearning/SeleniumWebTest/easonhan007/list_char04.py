#!/usr/bin/env python
# -*- coding:utf-8 -*-

test_list = [3,5,7,8,43,23,55,6,7,8]
def Odd(list):
    temp = []
    for i in range(0,len(list)):    #注意不能遗漏最后一个元素
        if i % 2 == 1:
            temp.append(list[i])
        else:
            pass
    return temp
print Odd(test_list)