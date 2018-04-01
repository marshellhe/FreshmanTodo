#!/usr/bin/env python
# -*- coding:utf-8 -*-
    
def log(func):
    def wrapper(*args,**kw):
        print 'call %s()'%func.__name__
        return func(*args,**kw)
    return wrapper#返回一个函数
    
@log#等同于log(now)
def now():
    print '2018-4-1'
#call = log(now)
#call()

def use_logging(level):
	def decorator(func):
		def wrapper(*args,**kw):
			if level == "warn":
				print "warnging log here"
			elif level == "info":
				print "info log here"
			return func(*args)
		return wrapper
	return decorator

@use_logging(level="warn")
def now2(name='need2'):
    print 'now2 executed',name

class Foo(object):
	def __init__(self,func):
		self._func = func

	def __call__(self):
		print "class decorator before func"
		self._func()
		print "class decorator after func"

@Foo
def bar():
	print "bar"
if __name__ =="__main__":
    #now2()
    bar()