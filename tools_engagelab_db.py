#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Date    : 2025-04-22 10:10:14
# @Author  : hejm (hejm@jiguang.cn)
# @Link    : ${link}
# @Version : $Id$

import os
from datetime import datetime, timedelta
from tcdb import TcDatabase

# host, port, user, password, database, = '10.55.68.82', 3306, 'kkpush', 'kkpush_ML@2021', 'sgp_apppush_kkpush'
host, port, user, password, = '10.55.68.82', 3306, 'username', 'pwd@2021'
database = 'sgp_apppush_pcdb'#sgp_apppush_pcdb,sgp_webpush_pcdb,usa_apppush_pcdb,usa_webpush_pcdb
tcdb = TcDatabase()
tcdb.connect(host, port, user, password, database)

    # ================== bsp conf for mysql 操作 ==================
def query_bsp_info(tcdb,bsp_id):
    target_table_name = f"t_bsp_info"
    sql = f"select * from {target_table_name} where bsp_id = '{bsp_id}';"
    # print(sql)
    exists = tcdb.mysql_result_assert(sql)
    # cur_now = datetime.now()
    # print(exists)
    if exists == "yes":
        print("got fix")
        val_dict = tcdb.select_one(target_table_name,"*", f"bsp_id = '{bsp_id}'")
        print(val_dict)
    else:
        print(f"not found specific data on database:{database}")

    # ================== bot conf for mysql 操作 ==================
def query_ai_bot_info(tcdb,app_key):
    target_table_name = f"t_ai_bot"
    sql = f"select * from {target_table_name} where app_key = '{app_key}';"
    # print(sql)
    exists = tcdb.mysql_result_assert(sql)
    # cur_now = datetime.now()
    # print(exists)
    if exists == "yes":
        print("got fix")
        val_dict = tcdb.select_one(target_table_name,"*", f"app_key = '{app_key}'")
        print(val_dict)
    else:
        print(f"not found specific data on database:{database}")

    # ================== app list for mysql 操作 ==================
def query_app_list(tcdb,app_key):
    target_table_name = f"t_app_list"
    sql = f"select * from {target_table_name} where app_key = '{app_key}';"
    # print(sql)
    exists = tcdb.mysql_result_assert(sql)
    # cur_now = datetime.now()
    # print(exists)
    if exists == "yes":
        print("got fix")
        val_dict = tcdb.select_one(target_table_name,"vip_status,privilege_start_ts,privilege_end_ts", f"app_key = '{app_key}'")
        print(val_dict)
    else:
        print(f"not found specific data on database:{database}")

import pymysql
from datetime import datetime, timedelta

def update_app_privilege(app_key, new_vip_status, days_to_add=None):
    """
    更新app的vip状态和特权结束时间
    :param app_key: 应用标识
    :param new_vip_status: 新vip状态(1,0,2,-1,-2,-3)
    :param days_to_add: 要增加的天数(None表示不修改结束时间)
    :return: 更新是否成功
    """
    if new_vip_status not in (1, 0, 2, -1, -2, -3):
        raise ValueError("Invalid vip_status value")
    
    conn = None
    try:
        # 创建数据库连接（请替换实际参数）
        conn = pymysql.connect(
            host=host,
            user=user,
            password=password,
            database=database,
            charset='utf8mb4',
            cursorclass=pymysql.cursors.DictCursor
        )
        
        with conn.cursor() as cursor:
            # 1. 查询当前状态
            sql = "SELECT vip_status, privilege_start_ts, privilege_end_ts FROM t_app_list WHERE app_key = %s"
            cursor.execute(sql, (app_key,))
            result = cursor.fetchone()
            
            if not result:
                print(f"App with key {app_key} not found")
                return False
            
            print("Current data:", result)
            
            # 2. 准备更新数据
            update_values = {'vip_status': new_vip_status}
            update_sql = "UPDATE t_app_list SET vip_status = %(vip_status)s"
            
            if days_to_add is not None:
                new_end_ts = result['privilege_end_ts'] + timedelta(days=days_to_add)
                update_values['privilege_end_ts'] = new_end_ts
                update_sql += ", privilege_end_ts = %(privilege_end_ts)s"
            
            update_sql += " WHERE app_key = %(app_key)s"
            update_values['app_key'] = app_key
            
            # 3. 执行更新
            cursor.execute(update_sql, update_values)
            conn.commit()
            
            print(f"Updated successfully. New vip_status: {new_vip_status}")
            if days_to_add:
                print(f"New privilege_end_ts: {update_values['privilege_end_ts']}")
            
            return True
            
    except Exception as e:
        print(f"Error occurred: {str(e)}")
        if conn:
            conn.rollback()
        return False
    finally:
        if conn:
            conn.close()

if __name__ == '__main__':
    app_key = "f5f29598e4a34fe9d6fbf3fb"#13598acb82a2ae496842c906,c1469fe8fe7d2fc489bcda0c
    # query_bsp_info(tcdb,app_key)
    query_ai_bot_info(tcdb,app_key)
    # query_app_list(tcdb,app_key)
    # update_app_privilege(app_key,-1)
    # update_app_privilege(app_key,-3,-30)