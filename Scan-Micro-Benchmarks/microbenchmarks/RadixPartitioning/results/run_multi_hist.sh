#!/usr/bin/env bash

echo "Radix Partitioning Bench"
for i in {0..9}
do
  echo "$i"
  ./radixpart --unroll=false --repeat=5 --user_check=true --min_radix_bits=0 --max_radix_bits=12 --s=65536000 --min_num_keys_exp=30 --max_num_keys_exp=30 | tee -a ./histogram.csv
  ./radixpart --unroll=true --repeat=5 --user_check=true --min_radix_bits=0 --max_radix_bits=12 --s=65536000 --min_num_keys_exp=30 --max_num_keys_exp=30 | tee -a ./histogram.csv
done
