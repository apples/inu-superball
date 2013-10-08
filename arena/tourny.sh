#!/bin/bash
set -e

./ai-gen 10 | while read line
do
    level=$(echo $line | wc -w)
    case $level in
    1)
        ;;
    2)
        #echo $line > galo-normal.txt
        #echo $line #':' $(./arena 6) >> results2.txt
        ;;
    3)
        if [ ! -f best2.txt ]
        then
            ./best < results2.txt | cut -d':' -f -1 > best2.txt
        fi
        if ./contains best2.txt "$line"
        then
            echo $line > galo-normal.txt
            result="$(./arena 6)"
            echo $line ':' $result
            echo $line ':' $result >> results3.txt
        fi
        ;;
    *)
        exit 1
        ;;
    esac
done
