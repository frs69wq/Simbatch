#!/bin/bash
cd /home/gcharrie/sources/Simbatch/example/task_reallocation
mkdir expes
#lance les expes avec des numeros de $1 a $2

makespan="sh $(pwd)/scritps/makespan.sh"
nbJobs=1000
frequency=(100 500 1000)
expMin=$1
expMax=$2

for (( i = $expMin; i <= $expMax; i++ ));
do
    #copie des fichiers de l'exp
    mkdir expes/example_$i
    cp *.c expes/example_$i
    cp *.h expes/example_$i
    cp Makefile expes/example_$i
    cp *.xml expes/example_$i
    #cp *.wld expes/example_$i
    cp *.txt expes/example_$i
    cp inputJobs/jobs$i.txt expes/example_$i/jobs.txt
    cd expes/example_$i
    echo -n "" > ./makespan.dat
    for resched in 0 1 2
    do 
	echo "  resched=${frequency[$resched]}"
	dossier=resched${frequency[$resched]}
	mkdir $dossier
#FCFS
	echo "    FCFS"
  #MCT
	echo "      MCT"
	make mrproper > /dev/null
	make
	mkdir $dossier/FCFS  
	mkdir $dossier/FCFS/MCT
	./example -f ./simbatch.xml
	mv *.out $dossier/FCFS/MCT
	mv realloc.txt $dossier/FCFS/MCT
	mv temps.txt $dossier/FCFS/MCT
#MCTRESCHED
	echo "      MCTRESCHED"
	sed 's/#-DRESCHED/-DRESCHED#/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	make mrproper > /dev/null
	make
	mkdir $dossier/FCFS/MCTRESCHED
	./example -f ./simbatch.xml
	mv *.out $dossier/FCFS/MCTRESCHED
	mv realloc.txt $dossier/FCFS/MCTRESCHED
	mv temps.txt $dossier/FCFS/MCTRESCHED
  #MINMIN
	echo "      MINMIN"
	sed 's/-DMCT/-DMINMIN/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	make mrproper > /dev/null
	make 
	mkdir $dossier/FCFS/MINMIN
	./example -f ./simbatch.xml 
	mv *.out $dossier/FCFS/MINMIN
	mv realloc.txt $dossier/FCFS/MINMIN
	mv temps.txt $dossier/FCFS/MINMIN
  #MAXMIN
	echo "      MAXMIN"
	sed 's/-DMINMIN/-DMAXMIN/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	make mrproper > /dev/null
	make 
	mkdir $dossier/FCFS/MAXMIN
	./example -f ./simbatch.xml
	mv *.out $dossier/FCFS/MAXMIN
	mv realloc.txt $dossier/FCFS/MAXMIN
	mv temps.txt $dossier/FCFS/MAXMIN
	
        #put makespans in a gnuplot file
	cd $dossier/FCFS/MCT
	echo -n "$($makespan) " >>  ../../../makespan.dat	
	cd ../MCTRESCHED
	echo -n "$($makespan) " >> ../../../makespan.dat
	cd ../MINMIN
	echo -n "$($makespan) " >> ../../../makespan.dat
	cd ../MAXMIN
	echo -n "$($makespan) " >> ../../../makespan.dat
	#retour dans example_$i
	cd ../../..
    
    #remise à zero
	sed 's/-DMAXMIN/-DMCT/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	sed 's/-DRESCHED#/#-DRESCHED/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	
#CBF
	echo "    CBF"
	sed 's/libfcfs.so/libcbf.so/g' simbatch.xml > SBtemp
	mv SBtemp simbatch.xml
  #MCT
	echo "      MCT"
	make mrproper > /dev/null
	make 
	mkdir $dossier/CBF
	mkdir $dossier/CBF/MCT
	./example -f ./simbatch.xml 
	mv *.out $dossier/CBF/MCT
	mv realloc.txt $dossier/CBF/MCT
	mv temps.txt $dossier/CBF/MCT
  #MCTRESCHED
	echo "      MCTRESCHED"
	sed 's/#-DRESCHED/-DRESCHED#/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	make mrproper > /dev/null
	make 
	mkdir $dossier/CBF/MCTRESCHED
	./example -f ./simbatch.xml
	mv *.out $dossier/CBF/MCTRESCHED
	mv realloc.txt $dossier/CBF/MCTRESCHED
	mv temps.txt $dossier/CBF/MCTRESCHED
  #MINMIN
	echo "      MINMIN"
	sed 's/-DMCT/-DMINMIN/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	make mrproper > /dev/null
	make 
	mkdir $dossier/CBF/MINMIN
	./example -f ./simbatch.xml
	mv *.out $dossier/CBF/MINMIN
	mv realloc.txt $dossier/CBF/MINMIN
	mv temps.txt $dossier/CBF/MINMIN
  #MAXMIN
	echo "      MAXMIN"
	sed 's/-DMINMIN/-DMAXMIN/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	make mrproper > /dev/null
	make 
	mkdir $dossier/CBF/MAXMIN
	./example -f ./simbatch.xml
	mv *.out $dossier/CBF/MAXMIN
	mv realloc.txt $dossier/CBF/MAXMIN
	mv temps.txt $dossier/CBF/MAXMIN
	
        #put makespans in a gnuplot file
	cd $dossier/CBF/MCT
	echo -n "$($makespan) " >> ../../../makespan.dat
	cd ../MCTRESCHED
	echo -n "$($makespan) " >> ../../../makespan.dat
	cd ../MINMIN
	echo -n "$($makespan) " >> ../../../makespan.dat
	cd ../MAXMIN
	echo -n "$($makespan) " >> ../../../makespan.dat
	#retour dans example_$i
	cd ../../..
	
#on remet FCFS, MCT et sans RESCHED
	sed 's/libcbf.so/libfcfs.so/g' simbatch.xml > SBtemp
	mv SBtemp simbatch.xml
	sed 's/-DMAXMIN/-DMCT/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	sed 's/-DRESCHED#/#-DRESCHED/g' Makefile > tmpMakefile
	mv tmpMakefile Makefile
	TMP="$(echo "($resched + 1) % 3" | bc)" 
	sed "s/deployment${frequency[$resched]}.xml/deployment${frequency[$TMP]}.xml/g" simbatch.xml > SBtemp
	mv SBtemp simbatch.xml
    done
    make mrproper > /dev/null 2> /dev/null
    
################################################################
# Fin de la simu, calculs des jobs passes plus tot
################################################################


#calcul du nombre de jobs passes plus tot
    sortie=./nbJobsEarlier.dat
    sortie1=./nbJobsSame.dat
    echo -n "" > $sortie
    #attention, le nb de jobs same est en fait jobsSame + jobsEarlier
    echo -n "" > $sortie1
    for resched in 0 1 2
    do
	FCFSj=$(cat ./resched${frequency[$resched]}/FCFS/MCT/*.out)
	for j in FCFS/MCTRESCHED FCFS/MINMIN FCFS/MAXMIN CBF/MCT CBF/MCTRESCHED CBF/MINMIN CBF/MAXMIN
	do
	    nbJobsSame=0
	    nbJobsEarlier=0
	    autre=$(cat ./resched${frequency[$resched]}/$j/*.out)
	    for (( job = 0; job < $nbJobs; job++ ))
	    do
		tempsFCFS=$(echo "$FCFSj" | grep "job$job " | cut -f 5)
		temps=$(echo "$autre" | grep "job$job " | cut -f 5)
		nbJobsEarlier=$(echo "if ($temps < $tempsFCFS) ($nbJobsEarlier + 1) else $nbJobsEarlier" | bc -l)
		nbJobsSame=$(echo "if ($temps == $tempsFCFS) ($nbJobsSame + 1) else $nbJobsSame" | bc -l)
	    done
	    TMP=$(echo "($nbJobsEarlier * 100) / $nbJobs" | bc -l)
	    echo -n "$TMP " >> $sortie
	    TMP=$(echo "(($nbJobsSame + $nbJobsEarlier) * 100) / $nbJobs" | bc -l)
	    echo -n "$TMP " >> $sortie1
	done
	echo "" >> $sortie
	echo "" >> $sortie1
    done

###############################################################
# Calcul du taux d'occupation de la plateforme
###############################################################

#calcul ratio utilisation de la plateforme et des temps d'execution des algos
    echo -n "" > tempsSomme.txt
    echo -n "" > tempsMax.txt
    echo -n "" > ./ratioUtilisation.txt
    for k in MCT MCTRESCHED MINMIN MAXMIN
      do    
      nb=0
      sommeTemps=0
      max=0
      for resched in 0 1 2
	do
	for j in FCFS CBF
	  do
	  oldPath=$(pwd)
	  cd ./resched${frequency[$resched]}/$j/$k
	  airetotale=$(echo "$($makespan) * 120" | bc -l)
	  somme=0
	  for ((toto = 0; toto < nbJobs; toto++))
	    do
	    duree=$(cat *.out | grep "job$toto " | cut -f 6)
	    nbProc=$(cat *.out | grep "job$toto " | cut -f 2)
	    TMP=$(echo "$duree * $nbProc + $somme" | bc -l)
	    somme=$TMP
	  done
	  ratio=$(echo "$somme / $airetotale * 100" | bc -l)
	  echo "$ratio " > ./ratioUtilisation.txt
		  
	  if [ $k != "MCT" ]
	      then
	      temps=$(cat ./temps.txt)
	      for ligne in $temps
		do
		TMP=$(echo "if ($ligne > $max) ($ligne) else ($max)" | bc -l)
		max=$TMP
		TMP=$(echo "$sommeTemps + $ligne" | bc -l)
		sommeTemps=$TMP
		((nb++))
	      done
	  fi
	  cd $oldPath
	done
      done
      if [ $k != "MCT" ]
	  then
	  echo -n "$sommeTemps $nb " >> tempsSomme.txt
	  echo -n "$max " >> tempsMax.txt
      fi
    done

    echo -n "" > ratioUtilisation.txt
    for resched in 0 1 2
      do
      for j in FCFS CBF
	do
	for k in MCT MCTRESCHED MINMIN MAXMIN
	  do
	  TMP=$(cat ./resched${frequency[$resched]}/$j/$k/ratioUtilisation.txt)
	  echo -n "$TMP " >> ratioUtilisation.txt
	done
      done
    done

    #retour dans task_reallocation
    cd ../..
done
