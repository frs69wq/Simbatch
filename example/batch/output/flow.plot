set term post eps enhanced "Times-Roman" 20
set encoding iso_8859_15
set clip points

set output "flow.eps"
# set title "Écart simulation/réalité"

# set grid
# set key below box
set style line 7 pointtype 5
set style line 2 pointtype 9 pointsize 1
set style line 1 linewidth 6 
#pointtype 7

set xlabel "Task's number"
set ylabel "Gap between Simbatch and OAR results"
set xrange [0:80000]
set yrange [0:3]

# fleche
# set label "Average flow" at 2,28
# set arrow 2 from 0.0,1.41370 to 100,1.41370 nohead ls 1


plot [:][:] 'flow.dat' index 0:0 using 1:2 with points title "error (%)" 
#ls 7,'flow.dat' index 1:1 using 1:2 with points title "CBF > FCFS" ls 2  
