#!/usr/bin/env python
# -*- coding:utf-8 -*-
"""
�� 0003 �⣺ �� 0001 �����ɵ� 200 �������루�����Ż�ȯ�����浽 Redis �ǹ�ϵ�����ݿ��С�
"""

import random
import string
import redis

forSelect = string.letters + '0123456789'

#����N���̶�����ַ����ļ����룬����list����,list��ÿ��Ԫ�����ַ���
def func_generate(num,length):
    re = []
    for i in range(num):
#        print '%s:'%(i+1) + ''.join(random.sample(forSelect,length))
        re.append(''.join(random.sample(forSelect,length)))
    return re
    """
#listת�������ɰ�����źͼ�������ɽ��tuple���б�
def func_convert(list):
    L = []
    for x in range(len(list)):
        L.append((x+1,list[x]))
    return L """

def StoreInRedis(convert_list):
    #����redis
    r = redis.Redis(host='127.0.0.1',port=6379,db=0)
    #forѭ�����String�е�ֵ
    id = 0
    id = 0
    for key in convert_list:
        id += 1
        r.set(id,key,nx=True)
        print (r.get(id))
        
if __name__ == '__main__':
    StoreInRedis(func_generate(200,20))