#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ���庯��
def temp_convert(var):
    try:
        return int(var)
    except ValueError,Argument:
        print "����û�а�������\n",Argument

#���ú���
temp_convert('xyz')