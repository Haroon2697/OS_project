#!/bin/bash

echo "=========================================="
echo "Neural Network Testing Script"
echo "=========================================="
echo ""

# Check if input file exists
if [ ! -f "input.txt" ]; then
    echo "ERROR: input.txt not found!"
    exit 1
fi

# Check if executable exists
if [ ! -f "neural_network" ]; then
    echo "ERROR: neural_network executable not found!"
    echo "Compiling..."
    gcc -o neural_network p.c -lpthread -lm
    if [ $? -ne 0 ]; then
        echo "Compilation failed!"
        exit 1
    fi
fi

echo "Test Configuration:"
echo "  - Hidden Layers: 2"
echo "  - Neurons per Layer: 8"
echo ""
echo "Running neural network simulation..."
echo "=========================================="
echo ""

# Run the program with automated input
printf "2\n8\n" | ./neural_network

echo ""
echo "=========================================="
echo "Checking output..."
echo "=========================================="

if [ -f "output.txt" ]; then
    echo "✓ output.txt created successfully"
    echo ""
    echo "First 30 lines of output.txt:"
    echo "----------------------------------------"
    head -30 output.txt
    echo ""
    echo "Last 10 lines of output.txt:"
    echo "----------------------------------------"
    tail -10 output.txt
    echo ""
    echo "File size: $(wc -l < output.txt) lines"
else
    echo "✗ ERROR: output.txt was not created!"
fi

echo ""
echo "=========================================="
echo "Test completed!"
echo "=========================================="

