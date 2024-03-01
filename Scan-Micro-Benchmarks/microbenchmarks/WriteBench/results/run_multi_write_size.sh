#!/usr/bin/env bash

echo "Write Size Bench"

export SGX_DBG_OPTIN=1

for i in {0..9}
do
  echo "$i"
  numactl --physcpubind="$i" --membind=0 -- nice -n -20 ./writebench --mode="write_size" --size_exp_min=35 --size_exp_max=35 --write_size_exp_min=0 --write_size_exp_max=20 --repeat=1  | tee -a ./write_size.csv
done
