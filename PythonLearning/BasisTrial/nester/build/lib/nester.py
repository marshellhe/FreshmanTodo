#!/usr/bin/env python
# -*- coding: utf-8 -*-

def prin_lol(the_list):
    for each_item in the_list:
        if isinstance(each_item,list):
            prin_lol(each_item)
        else:
            print(each_item)