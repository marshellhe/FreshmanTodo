#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import argv

script,custom_filename = argv
text = open(custom_filename)
print "current script file:%s"% script
print "Here 's your file %s:"%custom_filename
print text.read()
