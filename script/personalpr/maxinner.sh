#! /bin/bash

source script/personalpr/config.sh

ncpus="$NUMCPU"

echo -e "MaxInner\tStatic\tEager\tPrior"

for gg in webgoogle; do
	LOAD="-l $DATADIR/"$gg"_b100s"

	for inner in 1 2 3 4 5; do
		echo -ne $inner
		for schd in static eager; do
			EXEC="-r 1 $ncpus 100 $schd linear et $inner simplesweep convergence 10"
			TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
			echo -ne "\t$TIME"
		done
	
		for schd in maxsumprior; do
			EXEC="-r 1 $ncpus 100 $schd linear et $inner simplesweep convergence 10 normal InternalPrior"
			TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
			echo -ne "\t$TIME"
		done
		echo
	done
done

