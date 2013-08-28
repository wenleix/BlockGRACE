#! /bin/bash

source script/eik3d/config.sh

gg="meshc"
LOAD="-l $DATADIR/"$gg"_b5"

echo -e "Outer\tStaticInner\tDynamicInner"


declare -a schdname=("static" "eager" "prior0.05")
declare -a schdprint=("Static" "Eager" "Prior")

for i in 0 1 2; do
	schd="${schdname[$i]}"
	echo -ne "${schdprint[$i]}"

	EXTRA=""
	if [ "$schd" = "prior0.05" ]; then
		EXTRA="normal InternalPrior"
	fi

	for isch in simplesweep dynamicsweep; do
		for ncpus in $NUMCPU; do
			EXEC="-r 1 $ncpus 100 $schd linear et 100 $isch convergence 10 $EXTRA"
			TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
			echo -ne "\t$TIME"
		done
	done
	echo
done

