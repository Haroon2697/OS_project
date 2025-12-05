# How to Test Your Neural Network Project

## Current Status

✅ **Code is complete and compiles successfully**
✅ **Multi-process architecture implemented**
✅ **Pipe-based IPC working**
✅ **Thread-based neurons working**

⚠️ **Input file issue**: Your current `input.txt` only has 27 values, but for 2 hidden layers and 8 neurons, you need **466 values**.

## Testing Methods

### Option 1: Quick Architecture Test (Recommended First)

Test with smaller values to verify the architecture works:

```bash
cd /home/haroon/Downloads/i222697_SE-A_OS-Project

# Test with 1 hidden layer, 2 neurons (needs only ~30 values)
printf "1\n2\n" | ./neural_network
```

This will test:
- Process creation (fork)
- Pipe communication
- Thread execution
- Forward/backward passes

### Option 2: Check Input File First

Before running, verify your input file has enough data:

```bash
./check_input.sh
# Enter: 2 (hidden layers)
# Enter: 8 (neurons)
```

This will tell you if the file has enough values.

### Option 3: Manual Testing

```bash
./neural_network
# Then enter your configuration when prompted
```

### Option 4: Automated Test Script

```bash
./test.sh
```

## What You'll See When It Works

### Console Output:
```
*==================================================*
*  NEURAL NETWORK MULTI-CORE SIMULATOR            *
*  Process & Thread Based Architecture            *
*==================================================

CONFIGURATION INPUT
-------------------
Number of hidden layers (valid range 1-9): 2
Neurons per layer (valid range 1-100): 8

[STATUS] Configuration accepted.
[STATUS] Starting simulation with 2 hidden layers, 8 neurons/layer

[LAYER 0] INPUT LAYER (PID: xxxxx)
  Input neurons: 2
  Values: [1.2000, 0.5000]
  Output sent to next layer (processing complete)

[LAYER 1] HIDDEN LAYER (PID: xxxxx)
  Neurons: 8
  Processing complete

[LAYER 2] HIDDEN LAYER (PID: xxxxx)
  Neurons: 8
  Processing complete

[LAYER 3] OUTPUT LAYER (PID: xxxxx)
  Neurons: 8
  Processing complete

[PHASE] BACKWARD PROPAGATION (PID: xxxxx)
  Computing activation functions...
  Backward computation complete

[PHASE] SECOND FORWARD PASS
  Using backward outputs as new inputs...

[PHASE] SECOND FORWARD PASS - INPUT LAYER (PID: xxxxx)
  ...

*==================================================*
* SIMULATION FINISHED
* Results saved to output.txt
*==================================================
```

### Key Things to Verify:

1. **Multiple Process IDs**: Each layer should show a different PID
2. **No Error Messages**: Should not see "ERROR: Insufficient weight data"
3. **output.txt Created**: File should exist with complete results
4. **All Phases Complete**: Forward pass 1, backward pass, forward pass 2

## If You Get "Insufficient weight data" Error

This means your input file doesn't have enough weight values. You have two options:

### Option A: Create Complete Input File

Generate or obtain an input file with all required weights:
- For 2 hidden layers, 8 neurons: Need 466 total values
- Format: comma-separated values, can span multiple lines

### Option B: Modify Code to Reuse Weights (Quick Fix)

If you want to test with limited data, you could modify the second forward pass to reuse weights from the first pass. However, this is not ideal for the final submission.

## Testing Checklist

- [ ] Code compiles without errors
- [ ] Program runs without crashing
- [ ] Multiple processes are created (check PIDs)
- [ ] Pipes are working (no deadlocks)
- [ ] Threads are executing (neurons computing)
- [ ] Forward pass 1 completes
- [ ] Backward pass computes f(x1) and f(x2)
- [ ] Forward pass 2 completes
- [ ] output.txt is created with all results
- [ ] No memory leaks (use valgrind if available)

## Advanced Testing

### Check Running Processes:
```bash
# In another terminal while program runs:
watch -n 0.5 'ps aux | grep neural_network | grep -v grep'
```

### Check Output File:
```bash
cat output.txt
# or
less output.txt
```

### Memory Check (if valgrind installed):
```bash
printf "1\n2\n" | valgrind --leak-check=full ./neural_network
```

## Next Steps

1. **First**: Test with small configuration (1 layer, 2 neurons) to verify architecture
2. **Second**: Get/create complete input file with all weights
3. **Third**: Test with full configuration (2 layers, 8 neurons)
4. **Fourth**: Verify output.txt has all required information
5. **Fifth**: Prepare for demo with the provided test input file

## Files Created

- `neural_network` - Compiled executable
- `test.sh` - Automated test script
- `check_input.sh` - Input file verification script
- `TESTING_GUIDE.md` - Detailed testing documentation
- `HOW_TO_TEST.md` - This file

Good luck with your testing! 🚀

