#!/usr/bin/bash

for f in $1*.ply
do
    ./ply2asc < $f > ${f: : -4}"_ascii.ply"
done
