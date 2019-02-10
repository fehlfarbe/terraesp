#!/bin/bash

TERRAESP="terraesp.local"
echo "upload files to ${TERRAESP}"

for f in `ls data/*.{html,css,js}`
do
    echo "uploading ${f}..."
    curl -X POST -F "data=@${f}" http://${TERRAESP}/edit
    sleep 1
done