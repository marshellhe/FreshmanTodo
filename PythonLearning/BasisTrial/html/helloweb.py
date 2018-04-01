#!/usr/bin/env python
# -*- coding: utf-8 -*-
# helloweb.py
CONTENT_TYPE = 'text/html;charset=utf-8'

def application(environ,start_response):
    start_response('200 OK',[('Content-Type',CONTENT_TYPE)])
    unicodeText = u'<h1>Hello,小明!</h1>'
    return unicodeText.encode('utf-8')