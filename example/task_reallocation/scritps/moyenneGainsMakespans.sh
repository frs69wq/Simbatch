#!/bin/bash

#calcule la moyenne des gains pour chaque resched

frequency=(100 500 1000)
fichier=gainsMakespans.dat
sortie=moyenneGainsMakespans.dat
nbLignes=$(cat $fichier | wc -l)
echo -n "Exp FCFS/MCTR FCFS/MINMIN FCFS/MAXMIN " > $sortie
echo "CBF/MCT CBF/MCTR CBF/MINMIN CBF/MAXMIN"    >> $sortie
for resched in 0 1 2  
do
    echo -n "realloc=${frequency[$resched]} " >> $sortie
    for (( i = 1; i < 8; i++ ))
    do
	somme=0
	TMP="$(echo "($i + ($resched * 7)) + 1" | bc -l)"
	out=$(cat $fichier | cut -d " " -f $TMP)
        for line in $out
	do
	    TMP="$(echo "$somme + $line" | bc -l)"
	    somme=$TMP
	done
	TMP="$(echo "$somme / $nbLignes" | bc -l)"
	echo -n "$TMP " >> $sortie
    done
    echo "" >> $sortie
done
