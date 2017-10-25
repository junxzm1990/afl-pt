#!/usr/bin/env python2

import os
import sys
import argparse
import subprocess
import progressbar

def usage():
    print "Usage: python {0} -b SHOWMAP -p PARTITION_SCRIPT -d OUTDIR -e CMD -t interval_(hour) [-o tmp_out]" % sys.argv[0]
    print "do pip install progressbar2"

def main():
    exit_success = 0
    exit_failure = 1
    cargs = parse_cmdline()
    return not process_test_cases(cargs)

def get_edge_cov(show_map, seed_path, cov_cmd, tmp_out):
    #return a list of covered edge
    SUCCESS = 0
    cov = []
    if '|' in cov_cmd:
        cov_cmd = cov_cmd[cov_cmd.rindex('|')+1:]+' < '+ seed_path
    else:
        cov_cmd = cov_cmd.replace('AFL_FILE', seed_path)
    g_cmd = [show_map, '-e', '-T', '-o', tmp_out, cov_cmd]
    g_cmd = ' '.join(g_cmd)
    r = subprocess.call(g_cmd, shell=True)
    if r == SUCCESS:
        with open(tmp_out, 'r') as f:
            cov = f.read().splitlines()
        f.close()
    return cov



def partition(script, outdir, interval):
    #return how many partitions were created

    queue_path = "%s/queue"%outdir
    p_cmd = [script, queue_path, str(interval)]
    p_cmd = ' '.join(p_cmd)
    p = subprocess.Popen(p_cmd, stdout=subprocess.PIPE, shell=True)
    r = p.communicate()[0].strip()
    print "created %s partitions"%r
    return int(r)

def print_coverage_curve(curve, edge_set=None, out_file=None):
    for k, l in curve.items():
        print "{0}hr: {1} edges".format(k, len(l))

    if out_file is not None and edge_set is not None:
        with open(out_file, 'w') as f:
            f.write('\n'.join(list(edge_set)))
            f.close()
                


def process_test_cases(args):
    time_cov_curve = {}
    total_edge = set()

    #partition the target queue, return: par_num -- how many partition created
    par_num = partition(args.partition_script, args.afl_fuzzing_dir, args.interval) 



    bar = progressbar.ProgressBar(max_value=par_num)
    bar_count = 0
    #1 collect edge cov for each partition
    #1.1 for each input run afl-showmap on the target queue
    #2 update the curve_time_cov
    for p_time in range(args.interval, args.interval* (par_num +1), args.interval):
        edge_cov = set()
        target_par = "{0}-{1}hr/".format(os.path.dirname(args.afl_fuzzing_dir), p_time)
        if not os.path.exists(target_par):
            print target_par, "not exist"
            sys.exit(-1)
        for seed in os.listdir(target_par):
            seed_path = "%s/%s"%(target_par, seed)
            r_list = get_edge_cov(args.show_map, seed_path, str(args.coverage_cmd), args.tmp_out)
            edge_cov |= set(r_list)

        time_cov_curve[p_time] = set(edge_cov)
        total_edge |= set(edge_cov)
        bar_count += 1
        bar.update(bar_count)
    
    outfile = "%s/.edge_cov"%(args.afl_fuzzing_dir)
    print_coverage_curve(time_cov_curve,edge_set=edge_cov,out_file=outfile)
    os.remove(args.tmp_out)



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

    return p.parse_args()

if __name__ == "__main__":
    sys.exit(main())

