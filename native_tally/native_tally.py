#!/usr/bin/env python2

import os
import sys
import argparse
import subprocess
import progressbar
from multiprocessing import Pool
from operator import itemgetter

count = 0
cargs = argparse.ArgumentParser()
total_edge = set()

def usage():
    print "Usage: python {0} -b SHOWMAP -p PARTITION_SCRIPT -d OUTDIR -e CMD -t interval_(hour) [-o tmp_out] [-T timeout(ms)]" % sys.argv[0]
    print "do pip install progressbar2"

def main():
    exit_success = 0
    exit_failure = 1
    cargs = parse_cmdline()
    print cargs
    return not process_test_cases(cargs)

def get_edge_cov(show_map, seed_path, cov_cmd, tmp_out, timeout='3000'):
    #return a list of covered edge
    SUCCESS = 0
    cov = []
    if '|' in cov_cmd:
        cov_cmd = cov_cmd[cov_cmd.rindex('|')+1:]+' < '+ seed_path
    else:
        cov_cmd = cov_cmd.replace('AFL_FILE', seed_path)
    g_cmd = [show_map, '-e', '-T', '-m','none','-t',timeout+'+', '-o',  tmp_out, cov_cmd]
    g_cmd = ' '.join(g_cmd)
    # print g_cmd
    r = subprocess.call(g_cmd, shell=True)
    if r == SUCCESS:
        with open(tmp_out, 'r') as f:
            cov = f.read().splitlines()
        f.close()
    os.unlink(tmp_out)
    return cov



def partition(script, outdir, interval):
    #return how many partitions were created

    queue_path = "%s/queue"%outdir
    p_cmd = [script, queue_path, str(interval)]
    p_cmd = ' '.join(p_cmd)
    p = subprocess.Popen(p_cmd, stdout=subprocess.PIPE, shell=True)
    r = p.communicate()[0].strip()
    print p_cmd
    print "created %s partitions"%r
    return int(r)

def print_coverage_curve(curve, edge_set=None, out_file=None):
    for k, l in curve.items():
        print "{0}hr: {1} edges".format(k, len(l))

    if out_file is not None and edge_set is not None:
        with open(out_file, 'w') as f:
            f.write('\n'.join(list(edge_set)))
            f.close()
                


def process_one_seed(arguments):
    seed = arguments[0]
    cargs = arguments[1]
    target_par = "%s/queue"%cargs.afl_fuzzing_dir
    seed_path = "%s/%s"%(target_par, seed)
    seed_time = int(os.path.getmtime(seed_path))

    r_list = get_edge_cov(cargs.show_map, seed_path, str(cargs.coverage_cmd), cargs.tmp_out + seed, cargs.time_out)
    return (seed_time, set(r_list))


def process_test_cases(args):
    time_cov_curve = {}

    #partition the target queue, return: par_num -- how many partition created
#    par_num = partition(args.partition_script, args.afl_fuzzing_dir, args.interval) 



#    bar = progressbar.ProgressBar(max_value=par_num)
#    bar_count = 0
    #1 collect edge cov for each partition
    #1.1 for each input run afl-showmap on the target queue
    #2 update the curve_time_cov

    p = Pool(6)
    target_par = "%s/queue"%args.afl_fuzzing_dir
    listtest =  [("%s/%s"%(target_par, seed), args) for seed in os.listdir(target_par)]
    edge_set = p.imap(process_one_seed, [(seed, args) for seed in os.listdir(target_par)])



    cnt = 0;

    
    edge_sets = []
    for edge in edge_set:
        cnt += 1
	edge_sets.append(edge)
        if cnt % 500 == 0:
            print "Processed ", cnt 
		

    print "----- finished all coverage testing, now counting"
    edge_sets = sorted(edge_sets,key=itemgetter(0))
    print "----- now finished all sorting ", len(edge_sets)

    total_edges = set()	

    tdis = args.interval * 3600

    tstart = edge_sets[0][0]
    
    edge_cnt = 0

    for edge in edge_sets:
        tmp_edge_set = total_edges
        total_edges = total_edges.union(edge[1])
        edge_cnt += len(total_edges) - len(tmp_edge_set)         

        if edge[0] - tstart >= tdis:
		print "Number of edge in interval: ", edge_cnt 
		tstart = edge[0]

    if edge_cnt != 0:
        print "Number of edge in interval: ", edge_cnt

    return


def parse_cmdline():

    p = argparse.ArgumentParser()

    p.add_argument("-e", "--coverage-cmd", type=str, required=True,
            help="Set command to exec (including args, and assumes afl instrumentation support)")
    p.add_argument("-d", "--afl-fuzzing-dir", type=str, required=True,
            help="top level AFL fuzzing directory")
    p.add_argument("-b", "--show-map", type=str, required=True,
            help="the absolute path to afl-showmap binary")
    p.add_argument("-p", "--partition-script", type=str, required=True,
            help="the absolute path to the partition script")
    p.add_argument("-t", "--interval", type=int, required=True,
            help="Partition target input based on how many hours")
    p.add_argument("-o", "--tmp_out", type=str, required=False,
                   help="temporary file to store per input edges (recommend to put in ramdisk)", default='/tmp/.outout')
    p.add_argument("-T", "--time-out", type=str, required=False,
                   help="time out for each input case(ms)", default='3000')

    return p.parse_args()

if __name__ == "__main__":
    sys.exit(main())

