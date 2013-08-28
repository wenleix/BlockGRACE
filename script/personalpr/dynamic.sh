#! /bin/bash

source script/personalpr/config.sh

gg="webgoogle"
LOAD="-l $DATADIR/"$gg"_b100s"

echo -e "Outer\tStaticInner\tDynamicInner"


declare -a schdname=("static" "eager" "maxsumprior")
declare -a schdprint=("Static" "Eager" "Prior")

for i in 0 1 2; do
	schd="${schdname[$i]}"
	echo -ne "${schdprint[$i]}"

	EXTRA=""
	if [ "$schd" = "maxsumprior" ]; then
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

