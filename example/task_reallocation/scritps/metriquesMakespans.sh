#!/bin/bash
#calcule ecart type, min, max

frequency=(100 500 1000)
fichier=gainsMakespans.dat
sortie=metriquesMakespans.dat
nbLignes=$(cat $fichier | wc -l)
noms=(FCFS/MCTR FCFS/MINMIN FCFS/MAXMIN CBF/MCT CBF/MCTR CBF/MINMIN CBF/MAXMIN)
echo "Exp min max mean standardDeviation" > $sortie
for resched in 0 1 2
do
    for (( i = 1; i < 8; i++ ))
    do
	somme=0
	sommeCarres=0
	col=$(echo "($i + ($resched * 7)) + 1" | bc -l)
	out=$(cat $fichier | cut -d " " -f $col)
        for line in $out
	do
	    TMP=$(echo "$somme + $line" | bc -l)
	    somme=$TMP
	    TMP=$(echo "$sommeCarres + ($line * $line)" | bc -l)
	    sommeCarres=$TMP
	    
	done
	min=$(cat $fichier | cut -d " " -f $col | sort -n | head -n 1)
	max=$(cat $fichier | cut -d " " -f $col | sort -n | tail -n 1)
	mean=$(echo "$somme / $nbLignes" |bc -l)
	stdDeviation=$(echo "sqrt(($sommeCarres - (($somme * $somme) / $nbLignes)) / ($nbLignes - 1))" | bc -l)
	tmp=$(echo "$i - 1" | bc)
	ligne="${noms[$tmp]}/${frequency[resched]} $min $max $mean $stdDeviation"
	echo $ligne
	echo $ligne >> $sortie
    done
done
