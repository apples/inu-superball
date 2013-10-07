#!/bin/bash
set -e

./ai-gen 10 | while read line
do
    echo $line > galo-normal.txt
    echo $line > galo-panic.txt
    echo $line ':' $(./arena 6) >> results.txt
done
