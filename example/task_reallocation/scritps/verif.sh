#!/bin/bash

for (( j = 1; j < 101; j++))
do
  for i in 100 500 1000
    do
    for k in FCFS CBF
      do
      for l in MCT MCTRESCHED MINMIN MAXMIN
	do
	nbJobs=$(cat example_$j/resched$i/$k/$l/*.out | grep job | wc -l)
	if [ ! $nbJobs -eq 1000 ]
	then
	echo $j $i $k $l $nbJobs
	fi 
      done
      done
  done
done
