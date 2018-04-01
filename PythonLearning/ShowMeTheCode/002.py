#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
�� 0001 �⣺��Ϊ Apple Store App ���������ߣ���Ҫ����ʱ������Ϊ���Ӧ�����ɼ����루�����Ż�ȯ����ʹ�� Python ������� 200 �������루�����Ż�ȯ����
�� 0002 ��: �� 0001 �����ɵ� 200 �������루�����Ż�ȯ�����浽 MySQL ��ϵ�����ݿ��С�
˼·���������200�������룬��200����������Դ洢��һ��list
"""

import random
import string
import MySQLdb

forSelect = string.letters + '0123456789'

#����N���̶�����ַ����ļ����룬����list���棬list��ÿ��Ԫ�����ַ���
def func_generate(num,length):
    re = []
    for i in range(num):
#        print '%s:'%(i+1) + ''.join(random.sample(forSelect,length))
        re.append(''.join(random.sample(forSelect,length)))
    return re
#listת�������ɰ�����źͼ�������ɽ��tuple���б�
def func_convert(list):
    L = []
    for x in range(len(list)):
        L.append((x+1,list[x]))
    return L

def StoreInMySQL(test_list):
    try:
    # �����ݿ�����
        db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )
    # ʹ��cursor()������ȡ�����α� 
        cursor = db.cursor()
    except BaseException as e:
        print(e)
    else:
        try:
            #�������ݱ�SQL���
            sql1 = """CREATE TABLE ACTIVATE_KEY(KEY_ID INT PRIMARY KEY NOT NULL,KEY_VALUE VARCHAR(20))""" #VARCHAR�洢�䳤����
            sql2 = """INSERT INTO ACTIVATE_KEY (KEY_ID,KEY_VALUE) VALUES(%s,%s)"""
            cursor.execute("DROP TABLE IF EXISTS ACTIVATE_KEY")
            # ������ݱ��Ѿ�����ʹ�� execute()����ɾ����
            cursor.execute(sql1)
            #ִ��sql��䣬ʹ��executemany�����������Ĳ�������
            cursor.executemany(sql2,test_list)
            #��ѯ
            n = cursor.execute('SELECT * FROM ACTIVATE_KEY')
            print cursor.fetchall()
            db.commit() #�ύ�����ݿ�ִ�� 
        except:
            #Rollback in case there is any error
            db.rollback()
    finally:
        cursor.close()#�ر�ָ��/�α����
        db.close()   # �ر����ݿ�����     

if __name__ == '__main__':
    test = func_convert(func_generate(200,20))
    StoreInMySQL(test)