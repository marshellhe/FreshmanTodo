#!/usr/bin/env python
# -*- coding: utf-8 -*-
def customized_cmp():
    if x > y:
        return -1
    if x < y:
        return 1
    return 0
print(sorted([36,5]),customized_cmp)