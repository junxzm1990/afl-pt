#!/usr/bin/python

import os
import sys

def usage():
    print "Usage: python tally.py file_to_gcov_log path_to_test_case interval_(hours)"

def get_file_name(logline):
    part1 = logline.split("AFL test case:")[1]
    part2 = part1.split("(")[0]
    return part2.strip()

def get_time_stamp(path):
    return long(os.path.getmtime(path))

def get_func_cov(covline):
    part1 = covline.split(":")[1]
    part2 = part1.split("%")[0]
    return float(part2.strip())

def get_line_cov(covline):    
    part1 = covline.split(":")[1]
    part2 = part1.split("%")[0]
    return float(part2.strip())

def show_cov_line(cov_list, interval):
    start_time = cov_list[0][0]
    cov_winner_pool = {}

    for index in range(0, len(cov_list)):
        log = cov_list[index]
        time_slot = (log[0]-start_time)/3600/interval

        if time_slot not in cov_winner_pool.keys():
            cov_winner_pool[time_slot] = [log[1],log[2], log[3]]
        else:
            if cov_winner_pool[time_slot][0] < log[1]:
                cov_winner_pool[time_slot][0] = log[1]
                cov_winner_pool[time_slot][2] = log[3]
            if cov_winner_pool[time_slot][1] < log[2]:
                cov_winner_pool[time_slot][1] = log[2]
                cov_winner_pool[time_slot][2] = log[3]


    for i, v in cov_winner_pool.items():
        print "-- ", interval * (i+1), " hour function coverage ", v[0], " line coverage ", v[1]     
        print "\n"    


if __name__ == "__main__":
    if len(sys.argv) != 4:
        usage()
        exit(1)

    log_file = sys.argv[1]
    case_path = sys.argv[2]
    interval = int(sys.argv[3])

    lines = []
    cov_list = []

    with open(log_file, "r") as fr: 
        lines = fr.readlines()

    for index in range(0, len(lines)):
        if lines[index].find("AFL test case:") != -1:
            fname = get_file_name(lines[index])
            ftime = get_time_stamp(case_path+"/"+fname)
            for i in xrange(10):
                if 'functions' in lines[index+i]:
                    fcov = get_func_cov(lines[index+i])
                    break
            for i in xrange(10):
                if 'lines' in lines[index+i]:
                    lcov = get_line_cov(lines[index+i])
                    break
            cov_list.append([ftime, fcov, lcov, case_path+"/"+fname])




    show_cov_line(cov_list, interval)    
