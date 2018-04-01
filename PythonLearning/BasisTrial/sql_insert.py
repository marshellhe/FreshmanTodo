#!/usr/bin/python
# -*- coding: UTF-8 -*-

import MySQLdb

# 打开数据库连接
db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )

# 使用cursor()方法获取操作游标 
cursor = db.cursor()


#创建数据表SQL语句
sql = "INSERT INTO EMPLOYEE(FIRST_NAME,\
         LAST_NAME,AGE,SEX,INCOME) \
         VALUES ('%s','%s','%d','%c','%d')" %\
         ('Windows','King',25,'F',2450)
         
try:
    #执行sql语句
    cursor.execute(sql)
    #提交到数据库执行
    db.commit()
except:
    #Rollback in case there is any error
    db.rollback()


# 关闭数据库连接
db.close()