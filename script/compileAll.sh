#!/bin/bash

for app in personalpr sssp eik3d; do
	script/copyFromApp.sh $app
	make clean && make
	mv bin/GRACE.exec exec/$app
done



