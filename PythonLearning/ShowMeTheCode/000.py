#!/usr/bin/env python
# -*- coding: utf-8 -*-
from PIL import Image,ImageDraw,ImageFont
#import os
"""
**第 0000 题：**将你的 QQ 头像（或者微博头像）右上角加上红色的数字，类似于微信未读信息数量那种提示效果。 类似于图中效果
"""
#url = os.path.abspath

def add_num(img):
    draw = ImageDraw.Draw(img)
    myfont = ImageFont.truetype('C:\\Windows\\Fonts\\Arial.ttf',size = 40)
    fillcolor = '#ff0000'
    width,height = img.size
    draw.text((width-40,0),'4',font=myfont,fill=fillcolor)
    img.save('result.jpg','jpeg')
    return 0
 
if __name__ == '__main__':
    im = Image.open(r'C:\Users\Joyce\Desktop\pythonlearning\000\qq.png')
    add_num(im)
"""
print im.format,im.size,im.mode
im.show()
"""
