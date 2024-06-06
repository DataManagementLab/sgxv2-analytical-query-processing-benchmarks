# Settings for various experiments

## Dictionary scan

```shell
./simdmulti --enclave=b --preload=b --mode=dict --min_selectivity=10 --max_selectivity=10 --num_reruns=1 \
--unique_data=f --num_warmup_runs=10 --num_runs=1000 --min_threads=1 --max_threads=1 --min_entries_exp=10 \
--max_entries_exp=34
```

## In-Cache scan

```shell
./simdmulti --enclave=b --preload=b --mode=bitvector --min_selectivity=1 --max_selectivity=1 --num_reruns=1 \
--unique_data=f --num_warmup_runs=10 --num_runs=1000 --min_threads=1 --max_threads=1 --min_entries_exp=10 \
--max_entries_exp=34
```

## Out-of-cache scan

```shell
./simdmulti --enclave=b --preload=b --mode=noIndex,bitvector --min_selectivity=1 --max_selectivity=1 --unique_data=t \
--min_threads=1 --max_threads=1 --min_entries_exp=10 --max_entries_exp=34
```

## Scale-up

```shell
./simdmulti --enclave=b --preload=b --mode=bitvector --min_selectivity=1 --max_selectivity=1 --unique_data=t \
--min_threads=1 --max_threads=16 --min_entries_exp=34 --max_entries_exp=34
```

## Change write rate

```shell
./simdmulti --enclave=b --preload=b --mode=noIndex --min_selectivity=1 --max_selectivity=1 --unique_data=t \
--min_threads=16 --max_threads=16 --min_entries_exp=32 --max_entries_exp=32
./simdmulti --enclave=b --preload=b --mode=noIndex --min_selectivity=10 --max_selectivity=100 --step_selectivity=10 \
--unique_data=t --min_threads=16 --max_threads=16 --min_entries_exp=32 --max_entries_exp=32
```
