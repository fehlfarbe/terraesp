#!/bin/bash

cd "$( dirname "${BASH_SOURCE[0]}" )"

rm -rf data/www

# create directories
mkdir -p data/www/js
mkdir -p data/www/css

# copy and zip JS/CSS files
for d in js css
do
    for f in `ls www/${d}/*.${d}`
    do
        echo "Compress ${f} to data/${f}.gz"
        cat ${f} | gzip > data/${f}.gz
    done
done

# copy html
cat www/index.htm | gzip > data/www/index.htm.gz