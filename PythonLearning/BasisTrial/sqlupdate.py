#!/usr/bin/python
# -*- coding: UTF-8 -*-

import MySQLdb

# �����ݿ�����
db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )

# ʹ��cursor()������ȡ�����α� 
cursor = db.cursor()

#SQL�������
sql = "UPDATE EMPLOYEE SET AGE = AGE + 1 WHERE SEX = '%c'"%("M")
try:
    #ִ��SQL���
    cursor.execute(sql)
    #�ύ�����ݿ�ִ��
    db.commit()
except:
    #��������ʱ�ع�
    db.rollback()
