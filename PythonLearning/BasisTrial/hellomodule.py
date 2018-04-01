#!/usr/bin/env python
# -*- coding: utf-8 -*-
#以上两行是标准注释
""" a test module """
#模块代码的第一个字符串，表示模块的文档注释
__author__ ='Marshell He'
#写上本模块代码的作者
import sys
def test():
    args = sys.argv
#sys的argv变量，用list存储了命令行的所有参数。第一个参数永远是该.py文件的名称
    if len(args)==1:
        print 'Hello,world!'
    elif len(args)==2:
        print 'Hello,%s!'%args[1]
    else:
        print 'too many arguments!'

if __name__=='__main__':
    test()