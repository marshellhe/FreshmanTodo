#!/usr/bin/env python
# -*- coding:utf-8 -*-
"""
�� 0004 �⣺ ��һ��Ӣ�ĵĴ��ı��ļ���ͳ�����еĵ��ʳ��ֵĸ�����
˼·���򿪸��ļ���

str.strip([chars])
���������Ƴ��ַ���ͷβָ�����ַ�
split�����ַ����ķָ���Ĭ���Կո���зָ��������б�
������ʽ���Լ����Ż�
"""
import re

def File_CharsIntoList(file_path):
    L = []
    try:
        with open(file_path,'r') as f:
            #p = re.compile(r'\,|\.|\!|\:|\?|\n')
            for line in f.readlines():
                for new_line in line.split():
                    if ',' in new_line:
                        new_line = new_line.strip(',')
                    else: 
                        new_line = new_line
                    if '.' in new_line:
                        new_line = new_line.strip('.')
                    else:
                        new_line = new_line
                    L.append(new_line.strip('\n'))
    except IOError,e:
        raise IOError
    return L

def CharsIntoList_withRe(file_path):
    L = []
    try:
        with open(file_path,'r') as f:
            p = re.compile(r'\s+')
            for line in f.readlines():
                for i in p.split(line.strip('\n')):
                        L.append(i.strip(',').strip('.'))
                #L.append(t)
    except IOError,e:
        raise IOError
    return L
            
def CharsInListStatic(list):
    d = {}
    for i in range(len(list)):
        count = 0
        for lelement in list:
            if list[i] == lelement:
                count += 1
        d[list[i]] = count        
    print d    
if __name__ == '__main__':
    print File_CharsIntoList(r'C:\Users\Joyce\Desktop\pythonlearning\000\english.txt')
    print len(File_CharsIntoList(r'C:\Users\Joyce\Desktop\pythonlearning\000\english.txt'))
    print CharsIntoList_withRe(r'C:\Users\Joyce\Desktop\pythonlearning\000\english.txt')
    print len(CharsIntoList_withRe(r'C:\Users\Joyce\Desktop\pythonlearning\000\english.txt'))
    #CharsInListStatic(File_CharsIntoList(r'C:\Users\Joyce\Desktop\pythonlearning\000\english.txt'))
    #CharsInListStatic(File_CharsIntoList(r'C:\Users\Joyce\Desktop\pythonlearning\000\sketch.txt'))