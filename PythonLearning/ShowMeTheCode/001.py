#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
�� 0001 �⣺��Ϊ Apple Store App ���������ߣ���Ҫ����ʱ������Ϊ���Ӧ�����ɼ����루�����Ż�ȯ����ʹ�� Python ������� 200 �������루�����Ż�ȯ����
����˼·������200���̶����ȵ�����ַ���
"""

import random
import string

forSelect = string.letters + '0123456789'

def func_generate(num,length):
    re = []
    dict = {}
    for i in range(num):
#        print '%s:'%(i+1) + ''.join(random.sample(forSelect,length))
        re.append(''.join(random.sample(forSelect,length)))
    for j in range(len(re)):
        

if __name__ == "__main__":
    print func_generate(200,20)