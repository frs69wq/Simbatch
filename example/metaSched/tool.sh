#!/bin/bash

# Usage: scriptName oldWldFile
usage() {
    if [[ $# == 0 ]]
    then    
        echo "Usage: $0 file1.wld file2.wld ..."
        exit 1
    fi
}

# random floor ceil -> random integer in [floor, ceil[
random() {
    FLOOR=$1
    RANGE=$2
    number=0
    
    while [ "$number" -lt $FLOOR ]
    do
        number=$RANDOM
        ((number %= $RANGE))
    done

    return $number 
}

# old wld file format -> new wld file format 
# (add services + adapt nb_nodes) 
convert() {
    oldFile=$1
    newFile=$1.tmp
    while read submiT runT inD outD wallT nbNodes priority
    do
        random 1 3
        nb=$?
        random 1 6
        nbNodes=$?    
        line="$submiT \t $runT \t $inD \t $outD \t $wallT \t $nbNodes \t $priority \t Service$nb" 
        echo -e "$line" >> $newFile
    done < "$1"

    mv $newFile $oldFile
}

##### main ####

# init the random generator
SEED=1
RANDOM=$SEED

usage $@
for file in $@
do
    if [[ $file == *.wld ]]
    then
        # convert $file
        echo ok
    fi
done

