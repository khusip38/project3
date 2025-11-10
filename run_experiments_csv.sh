#!/bin/bash

# Output CSV
echo "Algorithm,NumFrames,PageFaultRate,TLBHitRate" > results.csv

# Array of frame sizes
frames=(32 64 128 256 512 768 1024)

# FIFO experiments
for f in "${frames[@]}"; do
    echo "Running FIFO with $f frames..."
    output=$(./main2 addresses.txt $f)  # pass num frames as argument
    pf=$(echo "$output" | grep "Page Fault Rate" | awk '{print $5}')
    tlb=$(echo "$output" | grep "TLB Hit Rate" | awk '{print $5}')
    echo "FIFO,$f,$pf,$tlb" >> results.csv
done

# LRU experiments
for f in "${frames[@]}"; do
    echo "Running LRU with $f frames..."
    output=$(./main3 addresses.txt $f)  # pass num frames as argument
    pf=$(echo "$output" | grep "Page Fault Rate" | awk '{print $5}')
    tlb=$(echo "$output" | grep "TLB Hit Rate" | awk '{print $5}')
    echo "LRU,$f,$pf,$tlb" >> results.csv
done

echo "All experiments completed. Results saved to results.csv"