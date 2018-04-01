#!/usr/bin/env python
# -*- coding: utf-8 -*-

name = 'Head First Python'

def what_happends_here():

    print name#SyntaxWarning:name 'name' is used prior to global declartion
    global name
    name = name + 'is a great book!'
    print name

what_happends_here()
print name