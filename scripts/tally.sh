#!/bin/bash


work_dir=

usage(){
    echo "Usage: $0 corpus_path time_interval(hr) aflcov_path gcov_obj_path 'gcov_obj_cmd' "
    echo ""
    echo "  This script take as input:"
    echo "    1) a fuzzer testcases directory"
    echo "    2) time inteval (unit: hour)"
    echo "    3) gcov compiled object directory"
    echo "    4) gcov program and cmdline (note: assuming the input fed to stdin)"
    echo ""
    echo "  It then output"
    echo "    1) N out_dir, each contains the inputs generated on the time-interval basis"
    echo "    2) It then run AFL-cov on each one of the out_dir, the result will be saved in-place"
}

get_timestamp(){
    fname=$1
    Timestamp=$(date --date="$fname" +"%s")
    echo "$Timestamp"
}

in_dir=$1
time_interval=$2
AFLCOV_PATH=$3
GCOV_OBJ_PATH=$4
PROG_CMD=$5

if [ -z $in_dir ]; then
    usage
    exit
fi

if [ -z $time_interval ]; then
    usage
    exit
fi

if [ -z $AFLCOV_PATH ]; then
    usage
    exit
fi

if [ -z $GCOV_OBJ_PATH ]; then
    usage
    exit
fi

# if [ -z ${PROG_CMD} ]; then
#     usage
#     exit
# fi



END_DATE=$(ls -lt $in_dir | grep $(whoami) | head -n1 | awk '{print $6" "$7" "$8}')
START_DATE=$(ls -lt $in_dir | grep $(whoami) | tail -n1 | awk '{print $6" "$7" "$8}')
END_DATE_STAMP=$(get_timestamp "$END_DATE")
START_DATE_STAMP=$(get_timestamp "$START_DATE")
SPAN=$((($END_DATE_STAMP-$START_DATE_STAMP)/3600))

echo "corpus start time: "$START_DATE
echo "corpus end time: "$END_DATE
echo "using interval as *"$time_interval"* hour(s)" 


setup_outdir(){
    for i in `seq $1 $1 $(($SPAN+$1))`; do
        skip=0
        out_dir=$(dirname $in_dir)'-'$i'hr'
        echo "setting up out_dir: "$out_dir
        mkdir $out_dir || skip=1
        if (( skip != 1)); then
            for f in $(ls $in_dir); do
                testcase=$in_dir'/'$f
                date=$(ls -lt $testcase | awk '{print $6" "$7" "$8}')
                time=$(get_timestamp "$date")
                curb=$((($time-$START_DATE_STAMP)/3600))
                echo $date $time $curb
                if (( curb <= i )); then
                    cp $testcase $out_dir
                fi
            done
            mkdir /tmp/queue
            mv $out_dir/* /tmp/queue
            mv /tmp/queue $out_dir
        fi
    done
}

collect_coverage(){
    for i in `seq $1 $1 $(($SPAN+$1))`; do
        out_dir=$(dirname $in_dir)'-'$i'hr'
        echo "testing out_dir: "$out_dir
        echo $PROG_CMD
        echo $CMD
        $AFLCOV_PATH/afl-cov -d $out_dir --coverage-cmd "cat AFL_FILE | $GCOV_OBJ_PATH/$PROG_CMD" --code-dir $GCOV_OBJ_PATH --overwrite
    done
}

setup_outdir $time_interval $in_dir
collect_coverage $time_interval

if [ ! -z $work_dir ]; then
    echo "saving results to "$work_dir
    for i in `seq $1 $1 $(($SPAN+$1))`; do
        out_dir=$(dirname $in_dir)'-'$i'hr'
        mv $out_dir $work_dir
    done
fi

