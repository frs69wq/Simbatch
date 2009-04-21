#!/bin/bash
# a lancer depuis task_reallocation

for (( i = $1; i <= $2; i++))
do
    echo "lancement expe $i"
    oarsub "$(pwd)/nodeLauncher.sh $i $i" -l nodes=1,walltime=12
done
