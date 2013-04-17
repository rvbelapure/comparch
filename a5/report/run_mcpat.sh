#!/bin/bash
for i in `ls *.xml`
do 
fname=`echo $i | cut -f 1 -d "."`
./mcpat -infile $i > $fname".mc" &
done
wait
