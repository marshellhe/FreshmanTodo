#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
第 0001 题：做为 Apple Store App 独立开发者，你要搞限时促销，为你的应用生成激活码（或者优惠券），使用 Python 如何生成 200 个激活码（或者优惠券）？
解题思路：生成200个固定长度的随机字符串
"""

import random
import string

forSelect = string.letters + '0123456789'

def func_generate(num,length):
    re = []
    dict = {}
    for i in range(num):
#        print '%s:'%(i+1) + ''.join(random.sample(forSelect,length))
        re.append(''.join(random.sample(forSelect,length)))
    for j in range(len(re)):
        

if __name__ == "__main__":
    print func_generate(200,20)