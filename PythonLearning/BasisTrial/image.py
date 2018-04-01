#!/usr/bin/env python
# -*- coding: utf-8 -*-

from PIL import Image
im = Image.open(r'C:\Users\Joyce\Desktop\RegularExpression.png')
print im.format,im.size,im.mode
im.thumbnail((200,100))
im.save(r'C:\Users\Joyce\Desktop\thumb.jpg','JPEG')