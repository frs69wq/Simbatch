#!/bin/bash

usage() {
    echo "Usage: $0 -n nbTests -p nbProcs -s nbServices -t nbTasks"
    exit 1
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
        echo -e "$stime \t ${runT[i]/.*} \t 0 \t 0 \t ${waitT/.*} \t ${nbNodes[i]/.*} \t 0 \t Service${services[i]/.*}" 
    } 
}


### main ###

nb=0
procs=5
services=2
tasks=0

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

if [[ $nb == 0 ]] && [[ $tasks == 0 ]]
then
    usage
fi

dir=xp2
if [ ! -e $dir ]
then
   mkdir $dir
fi
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
