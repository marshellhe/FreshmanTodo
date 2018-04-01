#!/usr/bin/env python
# -*- coding: utf-8 -*-

try:
    fh = open("testfileq","r")
    try:
        fh.write("this is a testing file for testing 异常！！")
    finally:
        print "关闭文件"
        fh.close()
except IOError:
    print "Error:没有找到文件或文件读取失败"