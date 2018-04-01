#!/usr/bin/python
# -*- coding: UTF-8 -*-

import MySQLdb

# 打开数据库连接
db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )

# 使用cursor()方法获取操作游标 
cursor = db.cursor()

# 如果数据表已经存在使用 execute()方法删除表
cursor.execute("DROP TABLE IF EXISTS HENRY")

#创建数据表SQL语句
sql = """CREATE TABLE HENRY(
         FIRST_NAME CHAR(20) NOT NULL,
         LAST_NAME CHAR(20),
         AGE INT,
         SEX CHAR(1),
         INCOME FLOAT)"""

cursor.execute(sql)

# 关闭数据库连接
db.close()