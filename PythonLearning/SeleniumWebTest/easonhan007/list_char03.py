#!/usr/bin/env python
# -*- coding:utf-8 -*-

test_list = [3,5,7,8,43,23,55,6,7,8]
def IsExist(i,list):
    if i not in list:
        return False
    else:
        return True
print IsExist(7,test_list)