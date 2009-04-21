set terminal postscript eps enhanced color
set output 'nbJobsEarlier.eps'
set style data histogram
set style histogram cluster gap 1
set style fill solid  border -1
set boxwidth 0.9
set key under
set ylabel "Percentage of jobs finished earlier than FCFS/MCT"
set yrange [0:100]
#set bmargin 5
set multiplot

plot 'jobsSame.dat' using 2:xtic(1) title col lc 9, \
	'' u 3 ti col lc 9, \
	'' u 4 ti col lc 9, \
	'' u 5 ti col lc 9, \
	'' u 6 ti col lc 9, \
	'' u 7 ti col lc 9, \
	'' u 8 ti col lc 9

plot 'jobsEarlier.dat' using 2:xtic(1) title col, \
	'' u 3 ti col, \
	'' u 4 ti col, \
	'' u 5 ti col, \
	'' u 6 ti col, \
	'' u 7 ti col, \
	'' u 8 ti col
set nomultiplot
