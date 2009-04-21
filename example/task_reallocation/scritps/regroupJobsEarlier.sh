#!/bin/bash

variable0=(0 0 0 0 0 0 0)
variable1=(0 0 0 0 0 0 0)
variable2=(0 0 0 0 0 0 0)
for (( i = $1; i <= $2; i++))
do
    for ((test = 1; test < 4; test++))
    do
	ligne=($(sed -n "$test ,$test p" ./example_$i/nbJobsEarlier.dat))
	for ((j = 0; j < 7; j++))
	do
	    if [ $test -eq 1 ]
	    then
		variable0[$j]=$(echo "${variable0[$j]} + ${ligne[$j]}" | bc -l)
	    fi
	    if [ $test -eq 2 ]
	    then
		variable1[$j]=$(echo "${variable1[$j]} + ${ligne[$j]}" | bc -l)
	    fi
	    if [ $test -eq 3 ]
	    then
		variable2[$j]=$(echo "${variable2[$j]} + ${ligne[$j]}" | bc -l)
	    fi
	done
    done
done

for ((j = 0; j < 7; j++))
do
    variable0[$j]=$(echo "${variable0[$j]} / ($2 - $1 + 1)" | bc -l)
    variable1[$j]=$(echo "${variable1[$j]} / ($2 - $1 + 1)" | bc -l)
    variable2[$j]=$(echo "${variable2[$j]} / ($2 - $1 + 1)" | bc -l)
done

sortie=jobsEarlier.dat
echo -n "Exp FCFS/MCTR FCFS/MINMIN FCFS/MAXMIN " > $sortie
echo "CBF/MCT CBF/MCTR CBF/MINMIN CBF/MAXMIN" >> $sortie
echo -n "realloc=100 " >> $sortie
for ((j = 0; j < 7; j++))
do
    echo -n "${variable0[$j]} " >> $sortie
done
echo "" >> $sortie
echo -n "realloc=500 " >> $sortie
for ((j = 0; j < 7; j++))
do
    echo -n "${variable1[$j]} " >> $sortie
done
echo "" >> $sortie
echo -n "realloc=1000 " >> $sortie
for ((j = 0; j < 7; j++))
do
    echo -n "${variable2[$j]} " >> $sortie
done
echo "" >> $sortie

variable0=(0 0 0 0 0 0 0)
variable1=(0 0 0 0 0 0 0)
variable2=(0 0 0 0 0 0 0)
for (( i = $1; i <= $2; i++))
do
    for ((test = 1; test < 4; test++))
    do
	ligne=($(sed -n "$test ,$test p" ./example_$i/nbJobsSame.dat))
	for ((j = 0; j < 7; j++))
	do
	    if [ $test -eq 1 ]
	    then
		variable0[$j]=$(echo "${variable0[$j]} + ${ligne[$j]}" | bc -l)
	    fi
	    if [ $test -eq 2 ]
	    then
		variable1[$j]=$(echo "${variable1[$j]} + ${ligne[$j]}" | bc -l)
	    fi
	    if [ $test -eq 3 ]
	    then
		variable2[$j]=$(echo "${variable2[$j]} + ${ligne[$j]}" | bc -l)
	    fi
	done
    done
done

for ((j = 0; j < 7; j++))
do
    variable0[$j]=$(echo "${variable0[$j]} / ($2 - $1 + 1)" | bc -l)
    variable1[$j]=$(echo "${variable1[$j]} / ($2 - $1 + 1)" | bc -l)
    variable2[$j]=$(echo "${variable2[$j]} / ($2 - $1 + 1)" | bc -l)
done

sortie=jobsSame.dat
echo -n "Exp FCFS/MCTR FCFS/MINMIN FCFS/MAXMIN " > $sortie
echo "CBF/MCT CBF/MCTR CBF/MINMIN CBF/MAXMIN" >> $sortie
echo -n "realloc=100 " >> $sortie
for ((j = 0; j < 7; j++))
do
    echo -n "${variable0[$j]} " >> $sortie
done
echo "" >> $sortie
echo -n "realloc=500 " >> $sortie
for ((j = 0; j < 7; j++))
do
    echo -n "${variable1[$j]} " >> $sortie
done
echo "" >> $sortie
echo -n "realloc=1000 " >> $sortie
for ((j = 0; j < 7; j++))
do
    echo -n "${variable2[$j]} " >> $sortie
done
echo "" >> $sortie
