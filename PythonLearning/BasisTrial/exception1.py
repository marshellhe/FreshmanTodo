#!/usr/bin/env python
# -*- coding: utf-8 -*-

try:
    fh = open("testfileq","r")
    try:
        fh.write("this is a testing file for testing �쳣����")
    finally:
        print "�ر��ļ�"
        fh.close()
except IOError:
    print "Error:û���ҵ��ļ����ļ���ȡʧ��"