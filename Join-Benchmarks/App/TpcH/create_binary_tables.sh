#!/usr/bin/env bash

for scale in 1 10
do
  for query in 3 10 12 19
  do
    ./csv_convert -s ${scale} -q ${query}
  done
done
