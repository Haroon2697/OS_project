# Submission Guidelines

## 📦 Submission Requirements

All submissions **MUST** be uploaded on **Google Classroom** strictly following deadlines. Solutions sent on emails will **NOT** be graded.

⚠️ **Important**: To avoid last-minute problems (unavailability of Google Classroom, load shedding, network down, etc.), you are **strongly advised** to start working on the project from day one.

---

## 📁 Submission Format

Each group will submit a **.zip file**, named according to the roll numbers of the group members.

**Example**: If group members have roll numbers `i222697`, `i222698`, `i222699`, the zip file should be named:
```
i222697_i222698_i222699.zip
```

---

## 📋 Contents of the .zip File

The zip file **MUST** contain the following:

### 1. All .cpp Source Files
- ✅ All C++ source code files (`.cpp` files)
- ✅ Header files if any (`.h` or `.hpp` files)
- ✅ **DO NOT** include:
  - Object files (`.o` files)
  - Executable files
  - Build artifacts
  - IDE-specific files

### 2. Project Report

A comprehensive project report covering the following sections:

#### **a. Title & Team Details**
- Project title
- Names of all group members
- Roll numbers of all group members
- Course information

#### **b. Problem Statement**
- Clearly define the problem being solved
- Explain what the neural network simulator does
- Describe the multi-process, multi-thread architecture

#### **c. System Design & Architecture**
- Describe how **layers are processes**
- Describe how **neurons are threads**
- Explain **IPC (Inter-Process Communication)** mechanism
- Explain **synchronization** mechanisms used
- Include architecture diagrams if possible

#### **d. Implementation Details**
- **Forward Pass**: How forward pass is implemented
- **Backward Pass**: How backward pass is implemented
- **Dynamic Process/Thread Creation**: How processes and threads are created at runtime
- **Input/Output Handling**: How input.txt is read and output.txt is written
- **Resource Management**: How pipes, mutexes, and memory are managed

#### **e. Sample Output**
- Include screenshots or tables showing:
  - Forward pass outputs (layer-wise)
  - Backward pass outputs (f(x1) and f(x2) values)
  - Second forward pass outputs
- Show terminal output with process IDs
- Show output.txt file contents

#### **f. Work Division**
- Describe each team member's contribution
- Who worked on which part
- How work was divided and coordinated

#### **g. Challenges Faced**
- Mention difficulties encountered during development
- How each challenge was solved
- What you learned from the challenges

---

## ✅ Pre-Submission Checklist

Before submitting, ensure:

- [ ] All `.cpp` files are included
- [ ] Code compiles without errors
- [ ] Code runs successfully with test input
- [ ] Project report is complete with all required sections
- [ ] Report is in PDF format (recommended) or Word format
- [ ] Sample outputs are included in the report
- [ ] Zip file is named correctly (roll numbers)
- [ ] Zip file size is reasonable (not too large)
- [ ] All files are readable and not corrupted

---

## 📝 File Structure Example

Your zip file should have a structure like this:

```
i222697_i222698_i222699.zip
├── project.cpp          (or main.cpp, neural_network.cpp, etc.)
├── input.txt            (optional - if you want to include test input)
├── PROJECT_REPORT.pdf   (or .docx)
└── README.txt           (optional - compilation instructions)
```

---

## 🔧 Compilation Instructions (for Report)

In your report, you may want to include compilation instructions:

```bash
# Compile the program
g++ -o neural_network project.cpp -lpthread -lm

# Run the program
./neural_network
```

---

## ⚠️ Common Mistakes to Avoid

1. ❌ **Wrong file name**: Not following roll number naming convention
2. ❌ **Missing source files**: Forgetting to include .cpp files
3. ❌ **Incomplete report**: Missing required sections
4. ❌ **No sample output**: Not including output screenshots/tables
5. ❌ **Corrupted files**: Files that cannot be opened
6. ❌ **Wrong format**: Submitting .c files instead of .cpp files
7. ❌ **Missing work division**: Not explaining team contributions
8. ❌ **Late submission**: Not submitting before deadline

---

## 📅 Important Reminders

- ⏰ **Deadline**: Check Google Classroom for exact deadline
- 📧 **No Email Submissions**: Only Google Classroom submissions accepted
- 🔄 **One Submission**: Only the latest submission will be graded
- ✅ **Test Before Submit**: Always test your code before submission
- 📋 **Complete Report**: Ensure all sections are covered

---

## 🎯 Grading Criteria Reference

Your submission will be evaluated based on:
- Code correctness and functionality
- Proper use of OS concepts (fork, pipe, pthread, mutex)
- Code quality and organization
- Completeness of project report
- Sample output correctness
- Work division clarity

---

## 📞 Need Help?

If you have questions about submission:
- Check Google Classroom announcements
- Review the project requirements document
- Contact your instructor during office hours

---

**Good luck with your submission!** 🚀

