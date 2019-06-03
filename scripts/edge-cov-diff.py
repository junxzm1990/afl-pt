#!/bin/env python2
import sys


s1 = set()
s2 = set()
with open(sys.argv[1], 'r') as f1:
    lines = f1.readlines()
    for i in lines:
        s1.add(i)
    f1.close()

with open(sys.argv[2], 'r') as f2:
    lines = f2.readlines()
    for i in lines:
        s2.add(i)
    f2.close()

print "overlap: %f"%(len(s1 & s2)/float(len(s1 | s2)))
print "only in %s: %f"%(sys.argv[1], len(s1 - s2)/float(len(s1 | s2)))
print "only in %s: %f"%(sys.argv[2], len(s2 - s1)/float(len(s1 | s2)))

