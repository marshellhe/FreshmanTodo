#!/usr/bin/env python
# -*- coding: utf-8 -*-

class Animal(object):
    def run(self):
        print 'Animal is running...'
        
class Dog(Animal):
    def run(self):
        print 'Dog is running...'
    
    def eat(self):
        print 'Eating meat...'

class Cat(Animal):
    def run2(self):
        print 'Cat is running2'