#!/bin/bash
# a lancer depuis expes

fichier="./ratiosUtilisation.dat"
echo -n "" > $fichier

for (( i = $1; i <= $2; i++))
do
    echo -n "$i " >> $fichier
    cat example_$i/ratioUtilisation.txt >> $fichier
    echo "" >> $fichier
done
