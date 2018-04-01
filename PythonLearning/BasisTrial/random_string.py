#!/usr/bin/env python
# -*- coding:utf-8 -*-

import random
import string

def func(a):
    str = 'abcedfghijklmnopqrstuvwxyz'
    list = []
    for i in range(0,a):
        list.append(str[random.randint(0,len(str)-1)])
    return ''.join(list)
print func(4)

def func_choice(b):
    list = []
    for i in range(0,b):
        list.append(random.choice(string.ascii_lowercase))
    return ''.join(list)
print func_choice(5)

def func_choice2(c):
    list = []
    for i in range(0,c):
        list.append(random.choice(string.letters))
    return ''.join(list)
print func_choice2(5)

def func_sample(d):
    return ''.join(random.sample(string.letters,d))
print func_sample(6)

def func_num(e):
    return ''.join(chr(random.randint(33,127)) for i in range(0,e))
print func_num(18)