#!/usr/bin/env bash

echo "Histogram Bench"
export SGX_DBG_OPTIN=1

result_file="./paper-histogram-data.csv"

for i in {0..2}
do
  echo "Run $i"
  numactl --physcpubind="$i" --membind=0 -- ./histogram --modes loop,simd_loop_unrolled --max_unrolling_factor 16 \
  --data_size=65536000 --max_radix_bits 15 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
  numactl --physcpubind="$i" --membind=0 -- ./histogram --modes loop,simd_loop_unrolled --max_unrolling_factor 16 \
  --data_size=65536000 --max_radix_bits 15 --ssb_mitigation \
  | tail -n +2 \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"
