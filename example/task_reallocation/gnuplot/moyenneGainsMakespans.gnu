set terminal postscript eps enhanced color "Times" 18
set output 'moyenneGainsMakespans.eps'
set style data histogram
set style histogram cluster gap 1
set style fill solid  border -1
set boxwidth 0.9
set key under
set ylabel "Gain (in %) compared to FCFS/MCT"
set yrange [0:]
plot 'moyenneGainsMakespans.dat' using 2:xtic(1) title col, \
	'' u 3 ti col, \
	'' u 4 ti col, \
	'' u 5 ti col, \
	'' u 6 ti col, \
	'' u 7 ti col, \
	'' u 8 ti col

