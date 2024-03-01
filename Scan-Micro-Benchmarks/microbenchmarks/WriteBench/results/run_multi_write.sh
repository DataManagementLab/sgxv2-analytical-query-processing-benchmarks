#!/usr/bin/env bash

echo "Write Bench"

export SGX_DBG_OPTIN=1

for i in {0..9}
do
  echo "$i"
  numactl --physcpubind="$i" --membind=0 -- ./writebench --mode="write" --repeat=1 --size_exp_min=3 --size_exp_max=35 | tee -a ./write_big.csv
done
