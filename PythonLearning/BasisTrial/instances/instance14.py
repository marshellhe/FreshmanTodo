#!/usr/bin/python
# -*- coding: UTF-8 -*-

for i in range(100,999):
    x = i/100
    y = i/10%10
    z = i%10
    if x ** 3 + y ** 3 + z ** 3 == i:
        print i