
for APPL in eik3d personalpr sssp; do
	for name in blocksize dynamic maxinner strategy; do
		echo Generating $name.dat for $APPL ...
		script/$APPL/$name.sh 2> /dev/null > result/$APPL/$name.dat
		echo Done
	done
done

for APPL in ukpr; do
	for name in blocksize maxinner strategy; do
		echo Generating $name.dat for $APPL ...
		script/$APPL/$name.sh 2> /dev/null > result/$APPL/$name.dat
		echo Done
	done
done


