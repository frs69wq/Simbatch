#!/bin/bash

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


##### main ####

# Usage: scriptName oldWldFile
if [[ $# != 1 ]]
then
    echo "Usage: $0 file"
    exit 1
fi

# parse & create the new wld file (with tmp suffix)
oldFile=$1
newFile=$1.tmp
while read submiT runT inD outD wallT nbNodes priority
do
    random 1 3
    nb=$?
    if [[ $nbNodes > 5 ]]
    then
       random 1 6
       nbNodes=$?
    fi    
    line="$submiT \t $runT \t $inD \t $outD \t $wallT \t $nbNodes \t $priority \t Service$nb" 
    echo -e "$line" >> $newFile
done < "$1"

# replace the old wlf file by the new one
mv $newFile $oldFile

