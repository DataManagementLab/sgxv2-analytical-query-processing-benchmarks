#!/usr/bin/python3

import csv
import argparse
import os


def convert_to_csv(input_file, output_file=None):
    # This list will contain dictionaries of parsed key-value pairs for each line
    data = []

    # Read the input file and parse it
    with open(input_file, 'r') as f:
        for line in f:
            parts = line.split()

            # Ignore lines that don't start with 'RESULT'
            if parts[0] != "RESULT":
                continue

            # Combine the second and third parts to form the datetime field
            parts[1] = parts[1] + " " + parts[2]
            del parts[2]

            # Create a dictionary for this line
            row_data = {}
            # For each key=value pair, split on '=' and add to dictionary
            for part in parts[1:]:
                if "=" not in part:
                    print(f"Warning: Skipping unexpected format '{part}' in file '{input_file}'")
                    continue
                key, value = part.split('=')
                row_data[key] = value
            data.append(row_data)

    # Extract fieldnames from the first dictionary, if data exists
    fieldnames = list(data[0].keys()) if data else []

    # If no output file specified, create one with .csv extension
    if not output_file:
        output_file = os.path.splitext(input_file)[0] + ".csv"

    # Use csv.DictWriter to write the dictionaries to a CSV file, if data exists
    if data:
        with open(output_file, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            for row in data:
                writer.writerow(row)


if __name__ == "__main__":
    #parser = argparse.ArgumentParser(description="Convert space delimited key=value pairs in text files to CSV.")
    #parser.add_argument("input_files", type=str, nargs="+", help="Input text files to convert.")
    #args = parser.parse_args()

    input_files = [f"data/pmbw_enclave_pointer_chasing_{n}.txt" for n in range(1, 11)]
    input_files += [f"data/pmbw_native_pointer_chasing_{n}.txt" for n in range(1, 11)]
    input_files += [f"data/pmbw_native_stats_large_{n}.txt" for n in range(1, 11)]
    input_files += [f"data/pmbw_enclave_stats_large_{n}.txt" for n in range(1, 11)]

    for input_file in input_files:
        convert_to_csv(input_file)

    print("Conversion completed.")
