#!/bin/bash
# check params and environment
if [ $# -ne 1 ]
then
echo "Usage: $0 [folder-name]"
exit 1
fi

cp app/$1/Graph.cpp src/
cp app/$1/Graph.h include/

