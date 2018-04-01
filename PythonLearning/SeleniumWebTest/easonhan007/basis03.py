#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
http://easonhan007.github.io/python/2013/12/27/py-simple-program
1~3
name = raw_input('请输入您的姓名！谢谢！')
if name == 'Alice' or name == 'Bob' or name == '何江闽':
    print 'Say hello to you,%s'%name
else:
    print 'Nothing left to you,good bye!'
"""
"""
4
number = int(raw_input('please input an integer number:'))
print sum(range(1,number+1))
"""
"""
5
number = int(raw_input('please input an integer number:'))
if number % 3 == 0 or number % 5 == 0:
    print sum(range(1,number+1))
elif number == 16:
    print 3+5+6+9+10+12+15
else:
    print 'nothing'
    """
"""
6
number = int(raw_input('please input an integer number:'))
choice = raw_input('you can choose "a" as summary or "b" as multiplation:')
if choice == 'a':
    print sum(range(1,number+1))
elif choice == 'b':
    print reduce(lambda x,y:x*y,range(1,number+1))
else:
    print nothing
"""
for i in range(1,13):
    s = ''  #该变量s不能定义在循环外面，不然执行次数过多
    for j in range(1,i+1):
        s += '%d*%d=%d '%(i,j,i*j)
    print s