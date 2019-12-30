#!/bin/bash

# set working dir
cd "$(dirname "$0")"

rm data/*

# concat all javascipt to one file
declare -a JSFiles=("data_uncompressed/angular.min.js" "data_uncompressed/angular-route.min.js" "data_uncompressed/highstock.js" "data_uncompressed/highslide-full.min.js" "data_uncompressed/highslide.config.js" )
rm data_uncompressed/app.js
for f in ${JSFiles[@]}
do
    echo "concat ${f}"
    (cat "${f}"; echo) >> data_uncompressed/app.js
done

# concat all CSS to one file
declare -a JSFiles=("data_uncompressed/bootstrap.min.css" "data_uncompressed/style.css" "data_uncompressed/highslide.css")
rm data_uncompressed/app.css
for f in ${JSFiles[@]}
do
    echo "concat ${f}"
    (cat "${f}"; echo) >> data_uncompressed/app.css
done

cp data_uncompressed/*.html data/
cp data_uncompressed/app.css data/
cp data_uncompressed/app.js data/

gzip -9 -f data/app.js
gzip -9 -f data/app.css