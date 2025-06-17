#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Date    : 2025-05-16 19:12:15
# @Author  : hejm (hejm@jiguang.cn)
# @Link    : ${link}
# @Version : $Id$

import os
from lib import MockTest
from lib import InitConn
from redistest import TcRedis
from lib import PushModel
from lib import BasicAuth

def get_send_token(uid, romtype=None):
    """
    func:根据uid获取厂商token
    params:
        uid - uid
    return: uid对应的厂商token
    """
    redis_conf = "10.55.68.83:6431","10.55.68.74:6432","10.55.68.98:6433"
    if  isinstance(redis_conf,(tuple,)):
        #redis cluster
        redisinst = TcRedis(redis_conf[0],redis_conf[1],redis_conf[2])

    else:
        #redis standalone
        redisinst = TcRedis(redis_conf)
    exists = redisinst.exists("dvc:"+str(uid))
    token = None
    if not exists:
        return None
    else:
        dict_str = redisinst.hgetall("dvc:"+str(uid))
        from datetime import datetime
        current_time = datetime.now().strftime("%H:%M:%S")
        print(f"当前时间 [时:分钟:秒]: {current_time}")
        print(f"key is dvc:{uid},keys:{dict_str.keys()}")
    if romtype == "fcm":
        if "fcm" in dict_str and "fcmToken" in dict_str["fcm"]:
            fcm = json.loads(dict_str["fcm"])
            token = fcm["fcmToken"]
    else:
        print("11111111")
        if "token" in dict_str and dict_str["token"]:
            print("3223424")
            token = dict_str["token"]
        else:
            print("no token found here")
    return token

if __name__ == '__main__':
    isTrue, uid, rid = InitConn.regAndroidInitConn('5')
    mock_reply = MockTest.mock_login(uid, rom='5')
    if mock_reply != 0:
        assert False
    import time
    time.sleep(1)
    paramDict = {
        "dataType": "message","msg.msg_content":"here is test"
    }
    tempConfig = {
        "appkey": "",
        "mascret": "",
        "package_name": "",
        "appkey_name": "",
        "headers": {"Authorization": BasicAuth.get_basic_auth_str("", ""),
                    "Content-Type": "application/json"},
        "push_target": rid,
        "uid": 12345,
        "small_icon_media_id": "",
        "small_icon_url": "",
        "large_icon_media_id": "",
        "large_icon_url": "",
        "rom": 0
    }
    pushModel = PushModel.PushModel(ver="v4")
    msgId, pushBody = pushModel.push_notification(tempConfig, paramDict)
    # sendTime = int(time.time())
    time.sleep(4)

    # time.sleep(8)
    print(get_send_token(uid))
