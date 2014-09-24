#!/bin/sh

for i in $(find -name \kpluginindex.json); do
	    echo "Deleting ... $i"
	    rm $i
    done
