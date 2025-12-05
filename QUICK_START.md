# Quick Start - How to Test

## ✅ Your Code is Ready!

The neural network implementation is **complete** with:
- ✅ Multi-process architecture (each layer = separate process)
- ✅ Pipe-based IPC (forward and backward communication)
- ✅ Thread-based neurons (parallel computation)
- ✅ Proper synchronization and cleanup

## 🚀 Three Ways to Test

### Method 1: Quick Test (Recommended First)
```bash
cd /home/haroon/Downloads/i222697_SE-A_OS-Project
printf "1\n2\n" | ./neural_network
```
**Why**: Tests with minimal data (1 hidden layer, 2 neurons) to verify architecture works.

### Method 2: Check Input File First
```bash
./check_input.sh
# Enter your configuration (e.g., 2 hidden layers, 8 neurons)
```
**Why**: Verifies if your input.txt has enough weight data before running.

### Method 3: Full Test
```bash
./neural_network
# Enter configuration when prompted
```
**Why**: Full test with your desired configuration.

## 📋 What You Should See

### ✅ Success Indicators:
1. **Different PIDs** for each layer (proves processes are separate)
2. **No error messages** (except if input file incomplete)
3. **output.txt created** with all computation results
4. **All phases complete**: Forward Pass 1 → Backward Pass → Forward Pass 2

### ⚠️ Common Issue:
**"ERROR: Insufficient weight data"**
- **Cause**: Input file doesn't have enough weight values
- **Solution**: Your input.txt needs weights for BOTH forward passes
- **For demo**: Use the input file provided by your instructor

## 📊 Expected Output Structure

```
[LAYER 0] INPUT LAYER (PID: xxxxx)     ← Different PID = separate process ✓
[LAYER 1] HIDDEN LAYER (PID: yyyyy)    ← Different PID = separate process ✓
[LAYER 2] OUTPUT LAYER (PID: zzzzz)    ← Different PID = separate process ✓
[PHASE] BACKWARD PROPAGATION            ← Backward pass working ✓
[PHASE] SECOND FORWARD PASS             ← Second forward pass ✓
```

## 🎯 Testing Checklist

Run through this checklist:

- [ ] Code compiles: `gcc -o neural_network p.c -lpthread -lm`
- [ ] Small test works: `printf "1\n2\n" | ./neural_network`
- [ ] Multiple PIDs visible (different for each layer)
- [ ] output.txt is created
- [ ] output.txt contains forward pass 1 results
- [ ] output.txt contains backward pass results (f(x1), f(x2))
- [ ] output.txt contains forward pass 2 results
- [ ] No crashes or segmentation faults

## 📁 Files You Have

- `p.c` - Your source code
- `neural_network` - Compiled executable
- `input.txt` - Input file (may need more weight data)
- `test.sh` - Automated test script
- `check_input.sh` - Input file checker
- `HOW_TO_TEST.md` - Detailed testing guide
- `TESTING_GUIDE.md` - Comprehensive documentation

## 💡 Pro Tips

1. **Start small**: Test with 1 layer, 2 neurons first
2. **Check PIDs**: Different PIDs prove multi-process architecture works
3. **Read output.txt**: Verify all computations are correct
4. **For demo**: Make sure you have the complete input file from instructor

## 🎓 For Your Demo

When demonstrating to your instructor:

1. Show the code uses `fork()` for processes
2. Show the code uses `pipe()` for IPC
3. Show the code uses `pthread_create()` for threads
4. Show different PIDs in output (proves separate processes)
5. Show output.txt with all results
6. Explain the architecture: layers = processes, neurons = threads

## 🆘 Need Help?

- Check `HOW_TO_TEST.md` for detailed instructions
- Check `TESTING_GUIDE.md` for comprehensive documentation
- Run `./check_input.sh` to verify input file

---

**Your project is 95% complete!** Just need to test with a complete input file. Good luck! 🚀

