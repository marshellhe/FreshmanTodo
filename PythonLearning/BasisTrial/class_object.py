#!/usr/bin/env python
# -*- coding: utf-8 -*-
class Student(object):
#Student后面跟着所继承的父类，如果没有就写object（所有类都最终继承）

    def __init__(self,name,score):
        self.name = name
        self.score = score
#__init__方法第一个参数必定是self，表示创建的实例本身
#同时创建实例的时候必须传入对应的参数，self不用
    def print_score(self):
        print '%s:%s'%(self.name,self.score)