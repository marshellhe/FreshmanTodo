#!/usr/bin/python
# -*- coding: UTF-8 -*-

import MySQLdb

# �����ݿ�����
db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )

# ʹ��cursor()������ȡ�����α� 
cursor = db.cursor()


#�������ݱ�SQL���
sql = "INSERT INTO EMPLOYEE(FIRST_NAME,\
         LAST_NAME,AGE,SEX,INCOME) \
         VALUES ('%s','%s','%d','%c','%d')" %\
         ('Windows','King',25,'F',2450)
         
try:
    #ִ��sql���
    cursor.execute(sql)
    #�ύ�����ݿ�ִ��
    db.commit()
except:
    #Rollback in case there is any error
    db.rollback()


# �ر����ݿ�����
db.close()