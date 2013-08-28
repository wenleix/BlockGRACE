#! /bin/bash

source script/personalpr/config.sh

gg="webgoogle"
echo -e "Schedule\tVertex\tVertexCA\tBlockS\tBlockCvg"

declare -a schdname=("static" "eager" "maxsumprior")
declare -a schdprint=("Static" "Eager" "Prior")

for i in 0 1 2; do
	metaschd="${schdname[$i]}"
	echo -ne "${schdprint[$i]}"
	for graph in "$gg"_randoms "$gg"_b100s; do
		LOAD="-l $DATADIR/$graph"
		schd="v_$metaschd"
		ncpus="$NUMCPU"
		EXEC="-r 1 $ncpus 100 $schd linear noet 1 simplesweep convergence 1000"
		TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
		echo -ne "\t$TIME"
	done

	EXTRA=""
	if [ "$metaschd" = "maxsumprior" ]; then
		EXTRA="normal InternalPrior"
	fi
	LOAD="-l $DATADIR/"$gg"_b100s"

	for inner in 1 100; do
		schd="$metaschd"
		EXEC="-r 1 $ncpus 100 $schd linear et $inner simplesweep convergence 10 $EXTRA"
		TIME=`echo -e "$LOAD\n$EXEC\n-e\n" | $GCMD | grep "^#" | cut -f 3 | awk 'NR == 3'`
		echo -ne "\t$TIME"
	done
	echo
done

