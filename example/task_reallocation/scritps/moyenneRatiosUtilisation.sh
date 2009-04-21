#!/bin/bash
#calcule la moyenne des ratios d'utilisation

input=./ratiosUtilisation.dat
output=./moyenneRatiosUtilisation.dat
metriquesOutput=./metriquesRatiosUtilisation.dat
taille=$(cat $input | wc -l)

sed 's/  / /g' $input > tempFile
mv tempFile $input


echo -n "Exp FCFS/MCT FCFS/MCTR FCFS/MINMIN FCFS/MAXMIN CBF/MCT " > $output
echo "CBF/MCTR CBF/MINMIN CBF/MAXMIN " >> $output
echo -n "realloc=100 " >> $output
echo -n "" > $metriquesOutput

for ((col = 2; col <= 25; col++))
do
  line=$(cat $input | cut -d " " -f $col)
  somme=0
  for val in $line
    do
    TMP=$(echo "$val + $somme" | bc -l)
    somme=$TMP
  done
  moyenne=$(echo "$somme / $taille" | bc -l)
  echo -n "$moyenne " >> $output
  if [ $col == "9" ]
      then
      echo -e -n "\n realloc=500 " >> $output
  fi
  if [ $col -eq 17 ]
      then
      echo -e -n "\n realloc=1000 " >> $output
  fi
done
