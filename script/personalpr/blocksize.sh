#! /bin/bash

source script/personalpr/config.sh

declare -a numgrab=("40" "20" "10" "5" "4" "3" "3" "3" "3")
declare -a blocksize=("25" "50" "100" "200" "300" "400" "800" "1200" "1600")

echo -e "BlokSize\tStatic\tEager\tPrior"

for gg in webgoogle; do
	for i in 0 1 2 3 4 5 6 7 8; do
		for inner in 100; do
			echo -ne "${blocksize[$i]}"
			for schd in static eager; do
				LOAD="-l $DATADIR/""$gg""_b${blocksize[$i]}s"
				EXEC="-r 1 $NUMCPU 100 $schd linear et $inner simplesweep convergence ${numgrab[$i]} normal noInternalPrior"
				TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
				echo -ne "\t$TIME"
			done

			schd="maxsumprior"
			LOAD="-l $DATADIR/""$gg""_b${blocksize[$i]}s"
			EXEC="-r 1 $NUMCPU 100 $schd linear et $inner simplesweep convergence ${numgrab[$i]} normal InternalPrior"
			TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
			echo -e "\t$TIME"
		done
	done
done

