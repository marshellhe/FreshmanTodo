#!/usr/bin/env python
# -*- coding:utf-8 -*-

str1 = 'abcdedcba'
str2 = '12344321'
str3 = 'awerewqfaesfewrqwerfqwe'
  
def ispalindrome(test_str):
    assert isinstance(test_str,basestring)
    if 0 <= len(test_str) <=1:
        return True
    elif test_str[0] != test_str[-1]:
        return False
    else:
        return ispalindrome(test_str[1:-1])
print ispalindrome(str1)
print ispalindrome(str2)
print ispalindrome(str3)
        
