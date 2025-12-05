#!/bin/bash

echo "=========================================="
echo "Input File Verification Script"
echo "=========================================="
echo ""

if [ ! -f "input.txt" ]; then
    echo "ERROR: input.txt not found!"
    exit 1
fi

echo "Reading configuration..."
read -p "Number of hidden layers: " H
read -p "Neurons per layer: " N

echo ""
echo "Calculating required values..."
INPUT_VALS=2
INPUT_LAYER_WEIGHTS=$((2 * N))
HIDDEN_LAYER_WEIGHTS=$((N * N))
OUTPUT_LAYER_WEIGHTS=$((N * N))

FIRST_PASS=$((INPUT_VALS + INPUT_LAYER_WEIGHTS + H * HIDDEN_LAYER_WEIGHTS + OUTPUT_LAYER_WEIGHTS))
SECOND_PASS=$((N * N + H * HIDDEN_LAYER_WEIGHTS + OUTPUT_LAYER_WEIGHTS))
TOTAL_NEEDED=$((FIRST_PASS + SECOND_PASS))

echo "First forward pass needs: $FIRST_PASS values"
echo "Second forward pass needs: $SECOND_PASS values"
echo "Total needed: $TOTAL_NEEDED values"
echo ""

echo "Counting values in input.txt..."
ACTUAL_COUNT=$(cat input.txt | tr ',' '\n' | grep -E '^[0-9]' | wc -l)
echo "Actual values in file: $ACTUAL_COUNT"
echo ""

if [ $ACTUAL_COUNT -ge $TOTAL_NEEDED ]; then
    echo "✓ Input file has enough values!"
else
    echo "✗ WARNING: Input file may not have enough values!"
    echo "  Missing: $((TOTAL_NEEDED - ACTUAL_COUNT)) values"
    echo ""
    echo "The program may fail during the second forward pass."
    echo "You may need to add more weight data to input.txt"
fi

echo ""
echo "=========================================="

