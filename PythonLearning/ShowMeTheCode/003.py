#!/usr/bin/env python
# -*- coding:utf-8 -*-
"""
第 0003 题： 将 0001 题生成的 200 个激活码（或者优惠券）保存到 Redis 非关系型数据库中。
"""

import random
import string
import redis

forSelect = string.letters + '0123456789'

#生成N个固定随机字符数的激活码，返回list保存,list中每个元素是字符串
def func_generate(num,length):
    re = []
    for i in range(num):
#        print '%s:'%(i+1) + ''.join(random.sample(forSelect,length))
        re.append(''.join(random.sample(forSelect,length)))
    return re
    """
#list转换函数成包含序号和激活码组成结对tuple的列表
def func_convert(list):
    L = []
    for x in range(len(list)):
        L.append((x+1,list[x]))
    return L """

def StoreInRedis(convert_list):
    #连接redis
    r = redis.Redis(host='127.0.0.1',port=6379,db=0)
    #for循环添加String中的值
    id = 0
    id = 0
    for key in convert_list:
        id += 1
        r.set(id,key,nx=True)
        print (r.get(id))
        
if __name__ == '__main__':
    StoreInRedis(func_generate(200,20))