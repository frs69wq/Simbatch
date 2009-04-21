#!/bin/bash
#calcule les gains de chaque algo par rapport a FCFS/MCT

fichier=makespans.dat
taille=$(wc -l $fichier)

echo -n "" > gainsMakespans.dat

cat $fichier |
while read line
do
    makespans=($line)
    echo -n "${makespans[0]} " >> gainsMakespans.dat
    tempsFCFSMCT=${makespans[1]}
    
    for (( i = 2; i < 25; i++ ))
    do
	if [ $i -eq 9 -o $i -eq 17 ] 
	then
	    echo "" > /dev/null
	else
	    TMP="$(echo "(($tempsFCFSMCT - ${makespans[i]})*100) / $tempsFCFSMCT" | bc -l)"
	    echo -n "$TMP " >> gainsMakespans.dat
	fi
    done
    echo "" >> gainsMakespans.dat
done

