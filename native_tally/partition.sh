#!/bin/bash


work_dir=

usage(){
    echo "Usage: $0 corpus_path time_interval(hr)"
    echo ""
    echo "  This script take as input:"
    echo "    1) a fuzzer testcases directory"
    echo "    2) time inteval (unit: hour)"
    echo ""
    echo "  It then output"
    echo "    1) N out_dir, each contains the inputs generated on the time-interval basis"
}

get_timestamp(){
    fname=$1
    Timestamp=$(date --date="$fname" +"%s")
    echo "$Timestamp"
}

in_dir=$1
time_interval=$2

if [ -z $in_dir ]; then
    usage
    exit
fi

if [ -z $time_interval ]; then
    usage
    exit
fi



END_DATE=$(ls --full-time $in_dir | grep $(whoami) | tail -n1 | awk '{print $6" "$7}')
START_DATE=$(ls --full-time $in_dir | grep $(whoami) | head -n1 | awk '{print $6" "$7}')
END_DATE_STAMP=$(get_timestamp "$END_DATE")
START_DATE_STAMP=$(get_timestamp "$START_DATE")
SPAN=$((($END_DATE_STAMP-$START_DATE_STAMP)/3600))

echo "corpus start time: "$START_DATE
echo "corpus end time: "$END_DATE
echo "using interval as *"$time_interval"* hour(s)" 


setup_outdir(){
    count=0
    for i in `seq $1 $1 $(($SPAN+$1))`; do
        skip=0
        out_dir=$(dirname $in_dir)'-'$i'hr'
        count=$(($count+1))
        # echo "setting up out_dir: "$out_dir
        mkdir $out_dir || skip=1
        if (( skip != 1)); then
            for f in $(ls $in_dir); do
                testcase=$in_dir'/'$f
                date=$(ls -lt $testcase | awk '{print $6" "$7" "$8}')
                time=$(get_timestamp "$date")
                curb=$((($time-$START_DATE_STAMP)/3600))
                # echo $date $time $curb
                if (( curb <= i )); then
                    cp $testcase $out_dir
                fi
            done
            # mkdir /tmp/queue
            # mv $out_dir/* /tmp/queue
            # mv /tmp/queue $out_dir
        fi
    done
    echo $count
}


setup_outdir $time_interval $in_dir

