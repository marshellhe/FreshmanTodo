#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""��100��������

for num in range(1,101):
    for i in range(2,num):
        if (num % i) == 0:
            break
    else:
        print num  

"""
"""�ж���ֵ
д�������������һ��������ʾ�û�����һ�����֣�����û�����ı��������ʾ�û����ڣ�������ʾ�û�С�ڣ�ֱ���û��µ������Ϊֹ��"""
"""
import random
a = random.randint(0,5)
b = int(raw_input("������һ������:")) 
while not b == a:
    if b < a:
        print '�������������%s����'%b
        b = int(raw_input("���������һ������:")) 
    else:
        print '�������������%s��С'%b
        b = int(raw_input("���������һ������:")) 
if b == a:
    print '������ȷ����ϲ����'
"""

for i in range(2017,2037):
    if (i % 4 == 0) and (i % 100 != 0) or (i % 100 == 0) and (i % 400 ==0):
        print '%s������'%i