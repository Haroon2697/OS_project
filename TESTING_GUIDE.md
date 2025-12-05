# Neural Network Testing Guide

## Quick Start Testing

### Method 1: Automated Test Script
```bash
cd /home/haroon/Downloads/i222697_SE-A_OS-Project
./test.sh
```

### Method 2: Manual Testing
```bash
cd /home/haroon/Downloads/i222697_SE-A_OS-Project
./neural_network
# When prompted:
# - Enter number of hidden layers (e.g., 2)
# - Enter neurons per layer (e.g., 8)
```

### Method 3: Automated Input (Non-Interactive)
```bash
printf "2\n8\n" | ./neural_network
```

## Expected Input File Structure

For a network with **H hidden layers** and **N neurons per layer**, the input file should contain:

1. **Line 1**: 2 input values (e.g., `1.2, 0.5`)
2. **Input Layer Weights**: 2 × N values (2 inputs × N neurons)
3. **Hidden Layer 1 Weights**: N × N values
4. **Hidden Layer 2 Weights**: N × N values
5. **... (for each hidden layer)**
6. **Output Layer Weights**: N × N values
7. **Second Pass Input Layer Weights**: N × N values (N backward inputs × N neurons)
8. **Second Pass Hidden Layer 1 Weights**: N × N values
9. **Second Pass Hidden Layer 2 Weights**: N × N values
10. **... (for each hidden layer)**
11. **Second Pass Output Layer Weights**: N × N values

### Example: 2 Hidden Layers, 8 Neurons
- Total values needed: 2 + (2×8) + (8×8)×3 + (8×8)×4 = 2 + 16 + 192 + 256 = **466 values**

## What to Check After Running

1. **Console Output**: Should show:
   - Process IDs for each layer
   - Status messages for each phase
   - No error messages

2. **output.txt File**: Should contain:
   - Forward Pass 1 results for all layers
   - Backward Pass computation (f(x1) and f(x2))
   - Forward Pass 2 results for all layers
   - "SIMULATION COMPLETED SUCCESSFULLY" message

3. **Process Verification**: Use `ps aux | grep neural_network` during execution to see multiple processes

## Common Issues

### Issue: "ERROR: Insufficient weight data"
**Cause**: Input file doesn't have enough weight values for the second forward pass.

**Solution**: 
- Check that your input file has weights for both forward passes
- Or modify the code to reuse first-pass weights for the second pass

### Issue: Program hangs
**Cause**: Deadlock in pipe communication or waiting for input.

**Solution**: 
- Check that all pipes are properly closed
- Verify input file format is correct

### Issue: Segmentation fault
**Cause**: Memory access errors or uninitialized pointers.

**Solution**: 
- Run with `gdb` to debug
- Check that all arrays are properly allocated

## Verification Commands

```bash
# Check if executable exists and is executable
ls -l neural_network

# Check input file
wc -l input.txt
head -5 input.txt

# Check output file
ls -l output.txt
head -20 output.txt
tail -20 output.txt

# Count processes during execution (in another terminal)
watch -n 1 'ps aux | grep neural_network | grep -v grep'
```

## Expected Output Structure

The `output.txt` should have this structure:
```
NEURAL NETWORK SIMULATION REPORT
=================================
Configuration: X Hidden Layers | Y Neurons Per Layer

FORWARD PASS 1 - INPUT LAYER COMPUTATION
Input: [value1, value2]
Output:
  Neuron[0] = ...
  Neuron[1] = ...
  ...

FORWARD PASS 1 - HIDDEN LAYER 1 COMPUTATION
Output:
  Neuron[0] = ...
  ...

FORWARD PASS 1 - OUTPUT LAYER COMPUTATION
Output:
  Output[0] = ...
  ...

BACKWARD PASS COMPUTATION
Formula 1: f(x1) = (x^2 + x + 1) / 2
Formula 2: f(x2) = (x^2 - x) / 2
Results:
  Neuron[0]: f(x1)=... | f(x2)=...
  ...

FORWARD PASS 2 - LAYER 1 OUTPUT
Output:
  Neuron[0] = ...
  ...

FORWARD PASS 2 - FINAL OUTPUT LAYER
Final Output:
  Output[0] = ...
  ...

SIMULATION COMPLETED SUCCESSFULLY
```

