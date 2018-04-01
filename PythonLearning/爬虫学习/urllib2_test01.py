#!/usr/bin/env python
# -*- coding: utf-8 -*-

import urllib2

"""test1
response = urllib2.urlopen('http://www.baidu.com/')
html = response.read()
print html
"""

request = urllib2.Request('http://www.baidu.com/')
response = urllib2.urlopen(request)
page = response.read()
print page
