#!/usr/bin/python
# -*- coding: UTF-8 -*-
import math
def panduan(x):
    if x > 1:
        for i in range(2,int(math.sqrt(x)+1)):
            if (x % i) == 0:
                return True
            else:
                return False
print filter(panduan,range(101))