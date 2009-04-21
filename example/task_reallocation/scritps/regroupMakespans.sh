#!/bin/bash
# a lancer depuis expes

fichier="./makespans.dat"
echo -n "" > $fichier

for (( i = $1; i <= $2; i++))
do
    echo -n "$i " >> $fichier
    cat example_$i/makespan.dat >> $fichier
    echo "" >> $fichier
done
