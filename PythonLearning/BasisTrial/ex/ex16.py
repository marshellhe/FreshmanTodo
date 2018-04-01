#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import argv

script,custom_filename = argv

print "we are going to erase %s." % custom_filename
print "If you don't want that,hit CTRL-C (^C)."
print "If you do want that,hit RETURN"

raw_input("?")

print "Opening the file..."
target = open(custom_filename,'a')

#print "Truncating the file.  goodbye!"
#target.truncate()

print "Now I'm going to ask you for giving us three lines"

line1 = raw_input("line1:")
line2 = raw_input("line2:")
line3 = raw_input("line3:")

print "those three lines will be there"
target.write(line1)
target.write("\n")
target.write(line2)
target.write("\n")
target.write(line3)
target.write("\n")

print "And Finally, we close it"
target.close()