#!/usr/bin/env python
# -*- coding: utf-8 -*-

import urllib
import urllib2

url = 'https://www.baibai.com'

values = {'source':'index_nav',
          'form_email':'13651498020', 
          'form_password':"Crowdtest133"}

data = urllib.urlencode(values)
req = urllib2.Request(url,data)
try:
    response = urllib2.urlopen(req)
except urllib2.HTTPError,e:
    print e,reason,e.code

