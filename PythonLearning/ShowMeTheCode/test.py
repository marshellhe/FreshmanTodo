#! /usr/bin/env python
# -*-coding:utf-8 -*-

def func_convert(list):
    L = []
    for x in range(len(list)):
        L.append((x+1,list[x]))
    return L

def file():
    try:
        f = open(r'C:\Users\Joyce\Desktop\pythonlearning\000\ShowMeTheCode.docx','rb')
        #�ļ�Ŀ¼·������ת��
        #ASCII�ļ�'r'����ASCII�ļ���'rb'
        print f.read()
    finally:
        if f:
            f.close()

if __name__ == '__main__':
    file()