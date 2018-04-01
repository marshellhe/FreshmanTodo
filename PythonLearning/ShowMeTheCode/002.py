#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
第 0001 题：做为 Apple Store App 独立开发者，你要搞限时促销，为你的应用生成激活码（或者优惠券），使用 Python 如何生成 200 个激活码（或者优惠券）？
第 0002 题: 将 0001 题生成的 200 个激活码（或者优惠券）保存到 MySQL 关系型数据库中。
思路：随机生成200个激活码，这200个激活码可以存储成一个list
"""

import random
import string
import MySQLdb

forSelect = string.letters + '0123456789'

#生成N个固定随机字符数的激活码，返回list保存，list中每个元素是字符串
def func_generate(num,length):
    re = []
    for i in range(num):
#        print '%s:'%(i+1) + ''.join(random.sample(forSelect,length))
        re.append(''.join(random.sample(forSelect,length)))
    return re
#list转换函数成包含序号和激活码组成结对tuple的列表
def func_convert(list):
    L = []
    for x in range(len(list)):
        L.append((x+1,list[x]))
    return L

def StoreInMySQL(test_list):
    try:
    # 打开数据库连接
        db = MySQLdb.connect("localhost","joyce","crowdtest123","test" )
    # 使用cursor()方法获取操作游标 
        cursor = db.cursor()
    except BaseException as e:
        print(e)
    else:
        try:
            #创建数据表SQL语句
            sql1 = """CREATE TABLE ACTIVATE_KEY(KEY_ID INT PRIMARY KEY NOT NULL,KEY_VALUE VARCHAR(20))""" #VARCHAR存储变长数据
            sql2 = """INSERT INTO ACTIVATE_KEY (KEY_ID,KEY_VALUE) VALUES(%s,%s)"""
            cursor.execute("DROP TABLE IF EXISTS ACTIVATE_KEY")
            # 如果数据表已经存在使用 execute()方法删除表
            cursor.execute(sql1)
            #执行sql语句，使用executemany方法来批量的插入数据
            cursor.executemany(sql2,test_list)
            #查询
            n = cursor.execute('SELECT * FROM ACTIVATE_KEY')
            print cursor.fetchall()
            db.commit() #提交到数据库执行 
        except:
            #Rollback in case there is any error
            db.rollback()
    finally:
        cursor.close()#关闭指针/游标对象
        db.close()   # 关闭数据库连接     

if __name__ == '__main__':
    test = func_convert(func_generate(200,20))
    StoreInMySQL(test)