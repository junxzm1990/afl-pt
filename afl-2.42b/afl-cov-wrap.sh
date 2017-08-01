#!/bin/bash
AFL_COV_PATH='/home/yaohui.c/work/afl-cov/'

if [ -z $1 ] || [ -z $2 ] || [-z $3 ]; then
    echo "Usage: $0 input_path gcov_obj_path 'gcov_obj_cmd'"
    exit
fi

INPUT_PATH=$1
GCOV_OBJ_PATH=$2
$GCOV_OBJ_CMD=$3




$AFL_COV_PATH/afl-cov -d $INPUT_PATH --coverage-cmd "cat AFL_FILE | $GCOV_OBJ_CMD" --code-dir $GCOV_OBJ_PATH --overwrite
