#!/bin/sh
if [ $# -lt 2 ]
then
    echo Usage $0 '<5 columns> <2 columns>'
    exit -1
fi
if [ ! -f $1 ]
then
    echo $1 : not a regular file.
    exit -1
fi
if [ ! -f $2 ]
then
    echo $2 : not a regular file.
    exit -1
fi

LIST=`grep -v '#' $1 | cut -f 4`
cat $2 | for v in $LIST; do read NUM VAL; echo $NUM $v $VAL; done
