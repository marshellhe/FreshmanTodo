#!/usr/bin/python
# -*- coding: UTF-8 -*-

import MySQLdb

# �����ݿ�����
db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )

# ʹ��cursor()������ȡ�����α� 
cursor = db.cursor()
#��ѯ���ݿ�汾
# ʹ��execute����ִ��SQL���
cursor.execute("SELECT VERSION()")

# ʹ�� fetchone() ������ȡһ�����ݿ⡣
data = cursor.fetchone()

print "Database version : %s " % data

# �ر����ݿ�����
db.close()