#!/usr/bin/env bash

for scale in 1 10 100
do
  ./csv_convert -s ${scale}
done
