#! /bin/bash

source script/ukpr/config.sh

declare -a numgrab=("10" "5" "3" "3" "3" "3")
declare -a blocksize=("100" "200" "400" "600" "800" "1200")

echo -e "BlokSize\tStatic\tEager\tPrior"

for gg in uk02o; do
	for i in 0 1 2 3 4 5; do
		for inner in 100; do
			echo -ne "${blocksize[$i]}"
			for schd in static eager; do
				LOAD="-fastload $DATADIR/""$gg""_b${blocksize[$i]}s"
				EXEC="-r 1 $NUMCPU 100 $schd linear et $inner simplesweep convergence ${numgrab[$i]} normal noInternalPrior"
				TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
				echo -ne "\t$TIME"
			done

			schd="maxsumprior"
			LOAD="-fastload $DATADIR/""$gg""_b${blocksize[$i]}s"
			EXEC="-r 1 $NUMCPU 100 $schd linear et $inner simplesweep convergence ${numgrab[$i]} normal InternalPrior"
			TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
			echo -e "\t$TIME"
		done
	done
done

