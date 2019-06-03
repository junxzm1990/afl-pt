#!/bin/env python2
import sys


path=sys.argv[1]+'/fuzzer_stats'
exec_period=0
start_time=0
cur_time=0
exec_num=0
with open(path, 'r') as f:
    lines = f.readlines()
    for line in lines:
        if 'start_time' in line:
            start_time=int(line.strip().split(':')[1])
        if 'last_update' in line:
            cur_time=int(line.strip().split(':')[1])
        if 'execs_done' in line:
            exec_num=int(line.strip().split(':')[1])

    f.close()

with open(sys.argv[2], 'a') as fout:
    fout.write("%d hour -- %s\n"%((cur_time-start_time)/3600, exec_num))
    fout.close()

