#!/usr/bin/python
#-*- coding:UTF-8 -*-
"""
��Ŀ�����ĸ����֣�1��2��3��4������ɶ��ٸ�������ͬ�����ظ����ֵ���λ�������Ƕ��٣�
"""
"""
#Function1

list = []
for i in range (1,5):
    for j in range(1,5):
        for k in range(1,5):
            if(i != k) and (i != j) and (j !=k):
                list.append([i,j,k])
print list
print "��������",len(list)
"""
"""
#Function2

list_num = [1,2,3,4]
list = [i*100 + j*10 + k for i in list_num for j in list_num for k in list_num if( i != j and j != k and k != i)]
print list
print "��������",len(list)
"""
"""
#Function3

line = []
for i in range(123,433):
    a = i%10
    b = (i%100)//10
    c = (i%1000)//100
    if (a != b) and (a!=c) and (b!=c) and (0<a<5)and (0<b<5)and (0<c<5):
        print i
        line.append(i)
print "��������",len(line)
"""
#Function4 

from itertools import permutations

j = 0
for i in permutations([1,2,3,4],3):
    print(i)
    j += 1
print "��������",j
