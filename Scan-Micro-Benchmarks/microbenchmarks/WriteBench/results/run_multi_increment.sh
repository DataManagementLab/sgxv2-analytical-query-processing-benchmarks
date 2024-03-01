#!/usr/bin/env bash

echo "Increment Bench"

export SGX_DBG_OPTIN=1

for i in {0..9}
do
  echo "$i"
  ./writebench --repeat=1 | tee -a ./increment.csv
done
