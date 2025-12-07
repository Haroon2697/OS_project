# Neural Network Multi-Core Simulator - Complete Guide

## 📋 Table of Contents
1. [How to Run the Project](#how-to-run-the-project)
2. [How to Give a Demo](#how-to-give-a-demo)
3. [Project Overview](#project-overview)
4. [OS Concepts Used](#os-concepts-used)
5. [Troubleshooting](#troubleshooting)

---

## 🚀 How to Run the Project

### Prerequisites
- Linux/Ubuntu system
- GCC compiler
- pthread library (usually pre-installed)

### Step 1: Compile the Project

```bash
cd /home/haroon/Downloads/i222697_SE-A_OS-Project
gcc -o neural_network p.c -lpthread -lm
```

**Expected Output**: No errors, executable `neural_network` created

### Step 2: Prepare Input File

Make sure `input.txt` exists in the project directory. The file should contain:
- **Line 1**: 2 input values (comma-separated), e.g., `1.2, 0.5`
- **Subsequent lines**: Weight values for all layers (comma-separated)

**Example for 2 hidden layers, 8 neurons:**
- Total values needed: 466 values
- Format: Comma-separated values, can span multiple lines

### Step 3: Run the Program

#### Method 1: Interactive Mode (Recommended for Demo)
```bash
./neural_network
```
Then enter:
- Number of hidden layers (1-9): `2`
- Neurons per layer (1-100): `8`

#### Method 2: Automated Input
```bash
printf "2\n8\n" | ./neural_network
```

#### Method 3: Using Test Script
```bash
./test.sh
```

### Step 4: Check Output

After execution, check:
1. **Console Output**: Should show process IDs and status messages
2. **output.txt**: Contains all computation results

```bash
cat output.txt
```

---

## 🎤 How to Give a Demo

### Pre-Demo Checklist

- [ ] Code compiles without errors
- [ ] `input.txt` file is ready (complete with all weights)
- [ ] Terminal is ready and visible
- [ ] `output.txt` is deleted (to show fresh run)
- [ ] You understand the architecture

### Demo Script (Step-by-Step)

#### **Part 1: Introduction (30 seconds)**

**Say:**
> "This is a multi-core neural network simulator that uses OS concepts like processes and threads. Each layer is a separate process, and each neuron is a thread, allowing parallel computation across multiple CPU cores."

#### **Part 2: Show the Code Structure (1-2 minutes)**

**Open `p.c` and show:**

1. **Process Creation (fork)**
   ```bash
   grep -n "fork(" p.c
   ```
   **Say:** "Here we use `fork()` to create separate processes for each layer. You can see 7 fork calls - one for input layer, one for each hidden layer, and one for output layer, plus second forward pass."

2. **Thread Creation (pthread)**
   ```bash
   grep -n "pthread_create" p.c
   ```
   **Say:** "Each neuron is a thread created with `pthread_create()`. This allows parallel computation within each layer."

3. **IPC with Pipes**
   ```bash
   grep -n "pipe(" p.c
   ```
   **Say:** "We use `pipe()` for inter-process communication. Processes communicate through pipes - forward pass pipes and backward pass pipe."

4. **Synchronization (Mutex)**
   ```bash
   grep -n "pthread_mutex" p.c | head -5
   ```
   **Say:** "We use mutex locks to prevent race conditions when multiple threads access shared resources like the output array or file."

#### **Part 3: Run the Program (2-3 minutes)**

**Execute:**
```bash
./neural_network
```

**Enter configuration:**
- Hidden layers: `2`
- Neurons per layer: `8`

**While it runs, point out:**
1. **Different PIDs**: "Notice each layer has a different Process ID (PID). This proves they are separate processes."
2. **Status Messages**: "You can see the progress of each layer processing."
3. **Process Creation**: "The system is creating processes dynamically based on our input."

**Show running processes (in another terminal):**
```bash
ps aux | grep neural_network | grep -v grep
```
**Say:** "You can see multiple processes running simultaneously - this is the multi-process architecture."

#### **Part 4: Show Output File (1 minute)**

**Open output.txt:**
```bash
cat output.txt
```

**Point out:**
1. **Forward Pass 1**: "Here are the results from the first forward pass through all layers."
2. **Backward Pass**: "This shows the backward propagation with f(x1) and f(x2) formulas."
3. **Forward Pass 2**: "The second forward pass uses backward outputs as new inputs."

**Show formulas:**
```bash
grep -A 2 "BACKWARD PASS" output.txt
```
**Say:** "The formulas f(x1) = (x² + x + 1)/2 and f(x2) = (x² - x)/2 are correctly implemented."

#### **Part 5: Explain Architecture (1-2 minutes)**

**Draw/Explain:**
```
Main Process (Parent)
├── Input Layer Process (fork) → PID: xxxxx
│   └── 2 Input Neurons (threads)
│   └── Pipe → Hidden Layer 1
├── Hidden Layer 1 Process (fork) → PID: yyyyy
│   └── 8 Neurons (threads)
│   └── Pipe → Hidden Layer 2
├── Hidden Layer 2 Process (fork) → PID: zzzzz
│   └── 8 Neurons (threads)
│   └── Pipe → Output Layer
└── Output Layer Process (fork) → PID: aaaaa
    └── 8 Neurons (threads)
    └── Backward Pipe → Input Layer
```

**Say:**
- "Each layer is a separate process created with fork()"
- "Each neuron within a layer is a thread created with pthread_create()"
- "Processes communicate via pipes - forward pass and backward pass"
- "Threads are synchronized with mutex to prevent race conditions"

#### **Part 6: Show OS Concepts (1 minute)**

**Quick verification:**
```bash
echo "=== OS Concepts Verification ==="
echo "Process Creation (fork): $(grep -c 'fork(' p.c)"
echo "IPC Pipes: $(grep -c 'pipe(' p.c)"
echo "Thread Creation: $(grep -c 'pthread_create' p.c)"
echo "Mutex Usage: $(grep -c 'pthread_mutex' p.c)"
echo "Process Wait: $(grep -c 'waitpid' p.c)"
```

**Say:** "The code uses all required OS concepts: process creation, IPC, threading, and synchronization."

#### **Part 7: Q&A Preparation**

**Be ready to answer:**

1. **"Why use processes instead of just threads?"**
   - Answer: "Processes provide better isolation and can utilize multiple CPU cores more effectively. Each process has its own memory space."

2. **"How do processes communicate?"**
   - Answer: "Through pipes. We create pipes with `pipe()` system call, and processes read/write data through these pipes."

3. **"What prevents race conditions?"**
   - Answer: "Mutex locks. Before accessing shared resources, threads acquire a mutex lock, preventing simultaneous access."

4. **"How is this different from a single-threaded program?"**
   - Answer: "Multiple processes and threads can execute in parallel on different CPU cores, significantly speeding up computation."

---

## 📊 Project Overview

### Architecture
- **Layers = Processes**: Each layer (input, hidden, output) is a separate process
- **Neurons = Threads**: Each neuron within a layer is a thread
- **IPC = Pipes**: Processes communicate via unnamed pipes
- **Synchronization = Mutex**: Threads use mutex locks for safe access

### Data Flow
1. **Forward Pass 1**: Input → Hidden Layers → Output (via pipes)
2. **Backward Pass**: Output computes f(x1), f(x2) → sends back via pipe
3. **Forward Pass 2**: Uses backward outputs as new inputs

### Key Features
- ✅ Dynamic process/thread creation based on user input
- ✅ Multi-core parallel processing
- ✅ Proper resource management (cleanup)
- ✅ Thread-safe operations
- ✅ Complete forward and backward passes

---

## 🔧 OS Concepts Used

### 1. Process Management
- **`fork()`**: Creates child processes for each layer (7 instances)
- **`waitpid()`**: Waits for child processes to complete (8 instances)
- **`getpid()`**: Gets process ID for identification
- **`exit()`**: Proper process termination

### 2. Inter-Process Communication (IPC)
- **`pipe()`**: Creates pipes for data transfer (15+ instances)
- **`read()`/`write()`**: Pipe data transfer
- **`close()`**: Closes file descriptors

### 3. Thread Management
- **`pthread_create()`**: Creates threads for neurons
- **`pthread_join()`**: Waits for threads to complete
- **`pthread_exit()`**: Thread termination

### 4. Synchronization
- **`pthread_mutex_t`**: Mutex lock type
- **`pthread_mutex_lock()`**: Acquires lock
- **`pthread_mutex_unlock()`**: Releases lock
- **`pthread_mutex_destroy()`**: Cleanup

---

## 🐛 Troubleshooting

### Issue: "ERROR: Input file not found"
**Solution**: Make sure `input.txt` exists in the project directory

### Issue: "ERROR: Insufficient weight data"
**Solution**: Your input file doesn't have enough weight values. For full test, you need:
- 2 hidden layers, 8 neurons: 466 total values
- Use the complete input file from your instructor

### Issue: Program hangs
**Solution**: 
- Check that all pipes are properly closed
- Verify input file format is correct
- Make sure you're using the correct number of layers/neurons

### Issue: Segmentation fault
**Solution**:
- Recompile: `gcc -o neural_network p.c -lpthread -lm`
- Check input file format
- Verify you entered valid numbers (1-9 for layers, 1-100 for neurons)

### Issue: No output.txt created
**Solution**:
- Check file permissions
- Make sure program completed successfully
- Check for error messages in console

---

## 📝 Quick Reference Commands

```bash
# Compile
gcc -o neural_network p.c -lpthread -lm

# Run
./neural_network

# Check processes during execution
ps aux | grep neural_network | grep -v grep

# View output
cat output.txt

# Count OS concepts
grep -c "fork(" p.c
grep -c "pipe(" p.c
grep -c "pthread_create" p.c
grep -c "pthread_mutex" p.c
```

---

## ✅ Demo Checklist

Before your demo:
- [ ] Code compiles successfully
- [ ] Input file is ready and complete
- [ ] You understand the architecture
- [ ] You can explain OS concepts
- [ ] Terminal is ready
- [ ] You've practiced the demo once

During demo:
- [ ] Show code structure (fork, pthread, pipe, mutex)
- [ ] Run the program
- [ ] Point out different PIDs
- [ ] Show output.txt
- [ ] Explain architecture
- [ ] Answer questions confidently

---

## 🎯 Key Points to Emphasize

1. **Multi-Process Architecture**: Each layer = separate process (fork)
2. **Multi-Threading**: Each neuron = thread (pthread)
3. **IPC**: Processes communicate via pipes
4. **Synchronization**: Mutex prevents race conditions
5. **Dynamic Creation**: Everything created at runtime based on user input
6. **Resource Management**: Proper cleanup of all resources

---

**Good luck with your demo! 🚀**
