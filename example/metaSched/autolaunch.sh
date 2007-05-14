#!/bin/bash

usage() {
    echo "Usage: $0 -n nbTests -p nbProcs -s nbServices -t nbTasks"
    exit 1
}

# initialize global variables with command line options
getOpts() {
    while getopts "n:p:s:t:" options
    do  
        case $options in 
            n ) nb=$OPTARG;;
            p ) procs=$OPTARG;;
            s ) services=$OPTARG;;
            t ) tasks=$OPTARG;;
            * ) echo unknown $options 
                usage;;
        esac 
    done
}

# check the number of procs and tasks 
checkOpts() {
    if [[ $1 == 0 ]] && [[ $2 == 0 ]]
    then
        usage
    fi
}

# Check that no directory will be erased
checkDir() {
    if [ ! -e $1 ]
    then
        mkdir $1
    else
        echo "Error: $1 already exist, delete or rename it" 
        exit 1
    fi
}

# generateLoad seed nbValue nbNodes nbServices -> file.wld
generateLoad() {
    local i runT submiT nbNodes services waitCoef stime 
    let tmp=$3+1
    submiT=(`gsl-randist $1 $2 poisson 300`) 
    runT=(`gsl-randist $1 $2 flat 600 1800`)
    nbNodes=(`gsl-randist $1 $2 flat 1 $tmp`)
    services=(`gsl-randist $1 $2 flat 0 $4`)
    #waitCoef=(`gsl-randist $1 $2 flat 1.1 3`)
    waitCoef=1
    stime=0
    for (( i=0; i<$2; ++i )) {
        let stime+=${submiT[i]/.*}
        waitT=$(echo "scale=1; ${waitCoef} * ${runT[i]}" | bc)
        # waitT=$(echo "scale=1; ${waitCoef[i]} * ${runT[i]}" | bc)
        echo -e "$stime \t ${runT[i]/.*} \t 0 \t 0 \t ${waitT/.*}"\
                "\t ${nbNodes[i]/.*} \t 0 \t Service${services[i]/.*}" 
    } 
}



### main ###

# global variables
nb=0
procs=5
services=2
tasks=0
dir=xp2

# init
getOpts $@
checkOpts $nb $tasks
checkDir $dir

cp *.xml $dir/
for bin in mct minMin minMax hpf
do
    for (( i=0; i<$nb; ++i )) {
        if [ ! -e $dir/$i ]
        then 
            mkdir $dir/$i
            generateLoad $i $tasks $procs $services  >> $dir/$i/load.wld
        fi
        cp $dir/$i/load.wld .
        `./$bin -f simbatch.xml > /dev/null`
        mkdir $dir/$i/$bin/
        mv Batch* $dir/$i/$bin
    }
done
rm -f load.wld
