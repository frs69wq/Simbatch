#!/bin/bash

echo -n "" > temps.dat
for l in MCTRESCHED MINMIN MAXMIN
do
  nb=0
  som=0
  max=0
  for i in 100 500 1000
    do
    for k in FCFS CBF
      do
	
      temps=$(cat example_$j/resched$i/$k/$l/temps.txt)
      for ligne in temps
	do
	max=$(echo "if ($ligne > $max) then $ligne else $max" | bc -l)
	som=$(echo "$som + $ligne" | bc -l)
	((nb++))
	done
    done
  done
  moy=$(echo "$sum / $nb" | bc -l)
  echo "$l $moy $max" >> temps.dat
done
