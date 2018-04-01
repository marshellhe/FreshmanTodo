#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
题目：暂停一秒输出。
程序分析：使用 time 模块的 sleep() 函数。
"""

import time

myID ={1:'a',2:'b'}
for key,value in dict.items(myID):
    print key,value
    time.sleep(2)