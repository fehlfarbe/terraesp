#!/bin/bash

TERRAESP="terraesp.local"
echo "upload files to ${TERRAESP}"

for f in `ls data`
do
    echo "uplading ${f}..."
    curl -X POST -F "data=@data/${f}" http://${TERRAESP}/edit
    sleep 1
done