#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import argv
from os.path import exists

script,source_filename,output_filename = argv

print "Opening the source file..."
src = open(source_filename,'r')

print "Does the output file exist? %s" % exists(output_filename)
out = open(output_filename,'w')

context = src.read()
out.write(context)

print "And Finally, we close it"
src.close()
out.close()