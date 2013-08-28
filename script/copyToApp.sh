#!/bin/bash
# check params and environment
if [ $# -ne 1 ]
then
echo "Usage: $0 [folder-name]"
exit 1
fi

cp src/Graph.cpp app/$1/ \
    && cp include/Graph.h app/$1/ 

if [ $? -eq 0 ]; then
    svn revert src/Graph.cpp
    svn revert include/Graph.h
else 
    echo Copy failed
fi

