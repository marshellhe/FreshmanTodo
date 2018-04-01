#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

print os.getcwd()
os.chdir(r'C:\Users\Joyce\Desktop\pythonlearning\HeadFirstPython\chapter3')
os.getcwd()
data = open('sketch.txt','r')
for each_line in data:

    print each_line.split('')
data.close()