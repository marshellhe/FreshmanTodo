#!/usr/bin/env python
# -*- coding: utf-8 -*-

import urllib2

req = urllib2.Request('https://www.baibai.com')

try:urllib2.urlopen(req)

except urllib2.HTTPError,e:
    print e.reason

