#!/usr/bin/env python
# -*- coding: utf-8 -*-

formatter = "%r %r %r %r"
#%r representation获取debug信息，获取变量的原始值，同标准输入输出%s不一样
print formatter % (1,2,3,4)
print formatter % ("one","two","three","four")
print formatter % (True,False,False,True)
print formatter % (formatter,formatter,formatter,formatter)
print formatter % (
    "I......",
    "b.....",
    "c....",
    "d...."
)