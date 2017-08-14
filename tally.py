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
	cur = cov_list[0][0]
	cnt = 1

	for log in cov_list:
		if log[0] > cur + interval * 3600:
			print "-- ", cnt * 4, " hour function coverage rage ", log.second.first, " line coverage ", log.second.second 	
			print "\n"	
			cnt += 1
			cur = log[0]

	while cnt <= 24 / interval:
		log = cov_list[-1]
		print "-- ", cnt * 4, " hour function coverage rage ", log[1], " line coverage ", log[2] 	
		print "\n"
		cnt += 1

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
			fcov = get_func_cov(lines[index+1])
			lcov = get_line_cov(lines[index+2])
			cov_list.append((ftime, fcov, lcov))
			
	show_cov_line(cov_list, interval)	
