#! /bin/bash

source script/eik3d/config.sh

declare -a numgrab=("37" "15" "8" "4" "3" "2")
declare -a len=("3" "4" "5" "6" "8" "10")
declare -a blocksize=("27" "64" "125" "216" "512" "1000")

gg="meshc"

echo -e "BlokSize\tStatic\tEager\tPrior"


for i in 0 1 2 3 4 5; do
	for inner in 100; do
		echo -ne "${blocksize[$i]}"
		for schd in static eager; do
			LOAD="-l $DATADIR/""$gg""_b${len[$i]}"
			EXEC="-r 1 $NUMCPU 100 $schd linear et $inner simplesweep convergence ${numgrab[$i]} normal noInternalPrior"
			TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
			echo -ne "\t$TIME"
		done

		schd="prior0.05"
		LOAD="-l $DATADIR/""$gg""_b${len[$i]}"
		EXEC="-r 1 $NUMCPU 100 $schd linear et $inner simplesweep convergence ${numgrab[$i]} normal InternalPrior"
		TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
		echo -e "\t$TIME"
	done
done

