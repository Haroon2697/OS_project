// Required header files - zaroori libraries (C++ style)
#include <cstdio>       // File operations ke liye (input/output)
#include <cstdlib>      // Memory allocation ke liye (malloc, free)
#include <cstring>      // String operations ke liye
#include <pthread.h>    // Threads banane ke liye (neurons threads hain)
#include <sys/wait.h>   // Process wait karne ke liye (waitpid)
#include <unistd.h>      // System calls ke liye (fork, pipe, close)
#include <semaphore.h>  // Synchronization ke liye
#include <fcntl.h>      // File control ke liye
#include <sys/stat.h>   // File status ke liye
#include <cerrno>       // Error handling ke liye

// Constants - fixed values
const int MAX_NEURONS = 100;      // Maximum neurons per layer
const int INPUT_NEURONS = 2;       // Input layer mein 2 neurons hain
const int BUFFER_SIZE = 8192;     // Buffer size for data transfer

// Thread data structure - har neuron thread ke liye data (C++ struct)
struct ComputeThread {
    int thread_id;              // Thread ka unique ID
    int input_size;             // Kitne inputs hain is neuron ko
    double *layer_inputs;       // Previous layer se aane wale inputs
    double *neuron_weights;     // Is neuron ke weights
    double *output_array;       // Output store karne ke liye array
    pthread_mutex_t *sync_lock; // Thread synchronization ke liye mutex
};

// Global variables - sab processes share karenge
FILE *result_file;                              // Output file pointer
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;  // File writing ke liye mutex

// Input file se comma-separated values parse karne ka function
// Yeh function input.txt se numbers read karta hai jo commas se separated hain
int parse_double_with_comma(FILE *fp, double *value) {
    // Valid number milne tak read karte raho
    while (1) {
        int c = fgetc(fp);
        if (c == EOF) return 0;  // File end ho gayi
        
        // Whitespace characters skip karo (space, tab, newline)
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        
        // Comma mila to skip karo aur aage badho
        if (c == ',') continue;
        
        // Character ko wapas file mein daalo aur number read karo
        ungetc(c, fp);
        if (fscanf(fp, "%lf", value) == 1) {
            // Trailing comma aur whitespace skip karo
            c = fgetc(fp);
            while (c == ',' || c == ' ' || c == '\t') {
                c = fgetc(fp);
            }
            if (c != EOF) ungetc(c, fp);
            return 1;  // Successfully number read ho gaya
        }
        return 0;  // Number read nahi ho saka
    }
}

// File exist karti hai ya nahi check karne ka function
int validate_file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: Input file '%s' not found or cannot be read\n", path);
        return 0;  // File nahi mili
    }
    fclose(f);
    return 1;  // File mil gayi
}

// Har neuron thread yeh function execute karta hai
// Yeh weighted sum calculate karta hai: sum = input1*weight1 + input2*weight2 + ...
void *execute_neuron_task(void *params) {
    ComputeThread *task = static_cast<ComputeThread *>(params);
    double sum = 0.0;
    
    // Sab inputs ko unke weights se multiply karke sum mein add karo
    for (int j = 0; j < task->input_size; j++) {
        sum += task->layer_inputs[j] * task->neuron_weights[j];
    }
    
    // Mutex lock karo taake output array safely update ho sake
    pthread_mutex_lock(task->sync_lock);
    task->output_array[task->thread_id] = sum;  // Result store karo
    pthread_mutex_unlock(task->sync_lock);
    
    pthread_exit(NULL);  // Thread complete
}

// Ek layer ke saare neurons ke liye threads create karta hai
// Har neuron parallel mein compute karega (multi-core advantage)
double* launch_neuron_threads(int num_neurons, int input_size, 
                              double *input_data, double *weights) {
    // Results store karne ke liye memory allocate karo
    double *results = static_cast<double *>(malloc(num_neurons * sizeof(double)));
    if (!results) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(1);
    }
    
    pthread_t tid_array[num_neurons];  // Thread IDs store karne ke liye
    pthread_mutex_t compute_lock = PTHREAD_MUTEX_INITIALIZER;  // Synchronization ke liye
    ComputeThread task_params[num_neurons];  // Har thread ke liye parameters
    
    // Har neuron ke liye thread create karo
    for (int i = 0; i < num_neurons; i++) {
        task_params[i].thread_id = i;
        task_params[i].input_size = input_size;
        task_params[i].layer_inputs = input_data;
        task_params[i].neuron_weights = &weights[i * input_size];  // Har neuron ke apne weights
        task_params[i].output_array = results;
        task_params[i].sync_lock = &compute_lock;
        
        // Thread create karo - yeh parallel mein execute hoga
        pthread_create(&tid_array[i], NULL, execute_neuron_task, &task_params[i]);
    }
    
    // Sab threads ke complete hone ka wait karo
    for (int i = 0; i < num_neurons; i++) {
        pthread_join(tid_array[i], NULL);
    }
    
    pthread_mutex_destroy(&compute_lock);  // Mutex cleanup
    return results;  // Sab neurons ke results return karo
}

// Pipe mein data write karne ka function (IPC - Inter-Process Communication)
// Ek process se doosre process ko data bhejne ke liye
int write_to_pipe(int pipe_fd, double *data, int count) {
    // Pehle count bhejo (kitne values hain)
    if (write(pipe_fd, &count, sizeof(int)) != sizeof(int)) {
        return 0;  // Write fail
    }
    // Phir actual data bhejo
    if (write(pipe_fd, data, count * sizeof(double)) != count * sizeof(double)) {
        return 0;  // Write fail
    }
    return 1;  // Success
}

// Pipe se data read karne ka function
// Doosre process se data receive karne ke liye
int read_from_pipe(int pipe_fd, double **data, int *count) {
    int n;
    // Pehle count read karo
    if (read(pipe_fd, &n, sizeof(int)) != sizeof(int)) {
        return 0;  // Read fail
    }
    
    *count = n;
    // Memory allocate karo data store karne ke liye
    *data = static_cast<double *>(malloc(n * sizeof(double)));
    if (!*data) {
        return 0;  // Memory allocation fail
    }
    
    // Actual data read karo
    if (read(pipe_fd, *data, n * sizeof(double)) != n * sizeof(double)) {
        free(*data);
        return 0;  // Read fail
    }
    return 1;  // Success
}

// Input layer process - yeh alag process hai (fork se create hua)
// Input layer 2 neurons se start hoti hai
void input_layer_process(int num_neurons, int neurons_per_layer, 
                        int write_fd, int layer_id) {
    printf("[LAYER %d] INPUT LAYER (PID: %d)\n", layer_id, getpid());
    printf("  Input neurons: %d\n", INPUT_NEURONS);
    
    // Is process ke liye output file kholo (append mode for thread-safe writing)
    FILE *local_result_file = fopen("output.txt", "a");
    if (!local_result_file) {
        fprintf(stderr, "ERROR: Cannot open output.txt\n");
        exit(1);
    }
    
    // Is process ke liye input file kholo
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        fclose(local_result_file);
        exit(1);
    }
    
    // Input values read karo (pehli line se)
    double input_values[INPUT_NEURONS];
    if (!parse_double_with_comma(input_fp, &input_values[0]) ||
        !parse_double_with_comma(input_fp, &input_values[1])) {
        fprintf(stderr, "ERROR: Failed to read initial input values\n");
        fclose(input_fp);
        exit(1);
    }
    
    printf("  Values: [%.4f, %.4f]\n", input_values[0], input_values[1]);
    
    // Weights read karo (2 inputs * num_neurons weights)
    double *weights = static_cast<double *>(malloc(INPUT_NEURONS * num_neurons * sizeof(double)));
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            exit(1);
        }
    }
    
    // Threads create karke computation karo (har neuron ek thread hai)
    double *output = launch_neuron_threads(num_neurons, INPUT_NEURONS, 
                                           input_values, weights);
    
    // Output file mein result write karo (mutex se protect karke)
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "FORWARD PASS 1 - INPUT LAYER COMPUTATION\n");
    fprintf(local_result_file, "Input: [%.6f, %.6f]\n", input_values[0], input_values[1]);
    fprintf(local_result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(local_result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(local_result_file, "\n");
    fflush(local_result_file);  // Ensure data is written
    pthread_mutex_unlock(&file_lock);
    
    // Next layer ko pipe se output bhejo (IPC)
    if (!write_to_pipe(write_fd, output, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write to pipe\n");
        exit(1);
    }
    
    // Cleanup - resources free karo
    close(write_fd);
    free(weights);
    free(output);
    fclose(input_fp);
    fclose(local_result_file);
    
    printf("  Output sent to next layer (processing complete)\n\n");
    exit(0);  // Process complete
}

// Hidden layer process - har hidden layer alag process hai
void hidden_layer_process(int layer_num, int num_neurons, 
                         int read_fd, int write_fd, int total_hidden_layers) {
    printf("[LAYER %d] HIDDEN LAYER (PID: %d)\n", layer_num, getpid());
    printf("  Neurons: %d\n", num_neurons);
    
    // Is process ke liye output file kholo (append mode)
    FILE *local_result_file = fopen("output.txt", "a");
    if (!local_result_file) {
        fprintf(stderr, "ERROR: Cannot open output.txt\n");
        exit(1);
    }
    
    // Is process ke liye input file kholo
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        fclose(local_result_file);
        exit(1);
    }
    
    // Pehle ki layers ke data skip karo (input values + input layer weights)
    double dummy;
    parse_double_with_comma(input_fp, &dummy);
    parse_double_with_comma(input_fp, &dummy);
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    
    // Previous hidden layers ke weights skip karo
    for (int i = 0; i < layer_num - 1; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    
    // Previous layer se pipe se input receive karo (IPC)
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        fclose(input_fp);
        exit(1);
    }
    close(read_fd);
    
    // Apne layer ke weights read karo
    double *weights = static_cast<double *>(malloc(input_count * num_neurons * sizeof(double)));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            fclose(input_fp);
            exit(1);
        }
    }
    
    // Threads create karke computation karo
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    // Output file mein result write karo
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "FORWARD PASS 1 - HIDDEN LAYER %d COMPUTATION\n", layer_num);
    fprintf(local_result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(local_result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(local_result_file, "\n");
    fflush(local_result_file);
    pthread_mutex_unlock(&file_lock);
    
    // Next layer ko pipe se output bhejo
    if (!write_to_pipe(write_fd, output, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write to pipe\n");
        exit(1);
    }
    
    // Cleanup
    close(write_fd);
    free(input_data);
    free(weights);
    free(output);
    fclose(input_fp);
    fclose(local_result_file);
    
    printf("  Processing complete\n\n");
    exit(0);
}

// Output layer process
void output_layer_process(int layer_num, int num_neurons, 
                         int read_fd, int backward_write_fd, int total_hidden_layers) {
    printf("[LAYER %d] OUTPUT LAYER (PID: %d)\n", layer_num, getpid());
    printf("  Neurons: %d\n", num_neurons);
    
    // Is process ke liye output file kholo (append mode)
    FILE *local_result_file = fopen("output.txt", "a");
    if (!local_result_file) {
        fprintf(stderr, "ERROR: Cannot open output.txt\n");
        exit(1);
    }
    
    // Open input file for this process
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        fclose(local_result_file);
        exit(1);
    }
    
    // Skip input values (2 values) and input layer weights
    double dummy;
    parse_double_with_comma(input_fp, &dummy);
    parse_double_with_comma(input_fp, &dummy);
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    
    // Skip all hidden layer weights
    for (int i = 0; i < total_hidden_layers; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        fclose(input_fp);
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            fclose(input_fp);
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "FORWARD PASS 1 - OUTPUT LAYER COMPUTATION\n");
    fprintf(local_result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(local_result_file, "  Output[%d] = %.6f\n", i, output[i]);
    }
    fprintf(local_result_file, "\n");
    fflush(local_result_file);
    pthread_mutex_unlock(&file_lock);
    
    printf("  Processing complete\n\n");
    
    // Backward pass computation - backpropagation simulate karna
    printf("[PHASE] BACKWARD PROPAGATION (PID: %d)\n", getpid());
    printf("  Computing activation functions...\n\n");
    
    // Backward data store karne ke liye memory
    double *backward_data = static_cast<double *>(malloc(num_neurons * sizeof(double)));
    if (!backward_data) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(1);
    }
    
    // Activation functions apply karo: f(x1) aur f(x2)
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "BACKWARD PASS COMPUTATION\n");
    fprintf(local_result_file, "Formula 1: f(x1) = (x^2 + x + 1) / 2\n");
    fprintf(local_result_file, "Formula 2: f(x2) = (x^2 - x) / 2\n");
    fprintf(local_result_file, "Results:\n");
    
    // Har neuron ke liye formulas apply karo
    for (int i = 0; i < num_neurons; i++) {
        double val = output[i];
        double fx1 = ((val * val) + val + 1.0) / 2.0;  // Formula 1
        double fx2 = ((val * val) - val) / 2.0;        // Formula 2
        
        backward_data[i] = fx1;  // Backward pass ke liye f(x1) use karo
        fprintf(local_result_file, "  Neuron[%d]: f(x1)=%.6f | f(x2)=%.6f\n", i, fx1, fx2);
    }
    fprintf(local_result_file, "\n");
    fflush(local_result_file);
    pthread_mutex_unlock(&file_lock);
    
    // Backward data ko pipe se previous layers ko bhejo
    if (!write_to_pipe(backward_write_fd, backward_data, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write backward data\n");
        exit(1);
    }
    close(backward_write_fd);
    
    free(input_data);
    free(weights);
    free(output);
    free(backward_data);
    fclose(input_fp);
    fclose(local_result_file);
    
    printf("  Backward computation complete\n\n");
    exit(0);
}

// Second forward pass - input layer
void second_input_layer_process(int num_neurons, int neurons_per_layer,
                                int read_backward_fd, int write_fd, int total_hidden_layers) {
    printf("[PHASE] SECOND FORWARD PASS - INPUT LAYER (PID: %d)\n", getpid());
    printf("  Using backward outputs as new inputs...\n\n");
    
    // Is process ke liye output file kholo (append mode)
    FILE *local_result_file = fopen("output.txt", "a");
    if (!local_result_file) {
        fprintf(stderr, "ERROR: Cannot open output.txt\n");
        exit(1);
    }
    
    // Open input file for this process
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        fclose(local_result_file);
        exit(1);
    }
    
    // Skip: input values + input layer weights + all hidden layer weights + output layer weights
    double dummy;
    parse_double_with_comma(input_fp, &dummy);
    parse_double_with_comma(input_fp, &dummy);
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    for (int i = 0; i < total_hidden_layers; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    for (int i = 0; i < num_neurons * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    
    // Read backward data
    double *backward_data;
    int backward_count;
    if (!read_from_pipe(read_backward_fd, &backward_data, &backward_count)) {
        fprintf(stderr, "ERROR: Failed to read backward data\n");
        fclose(input_fp);
        exit(1);
    }
    close(read_backward_fd);
    
    // Read weights for second pass
    double *weights = static_cast<double *>(malloc(backward_count * num_neurons * sizeof(double)));
    for (int i = 0; i < backward_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data for second pass\n");
            fclose(input_fp);
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, backward_count, 
                                           backward_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "FORWARD PASS 2 - LAYER 1 OUTPUT\n");
    fprintf(local_result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(local_result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(local_result_file, "\n");
    fflush(local_result_file);
    pthread_mutex_unlock(&file_lock);
    
    // Send output through pipe
    if (!write_to_pipe(write_fd, output, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write to pipe\n");
        exit(1);
    }
    
    close(write_fd);
    free(backward_data);
    free(weights);
    free(output);
    fclose(input_fp);
    fclose(local_result_file);
    
    exit(0);
}

// Second forward pass - hidden layer
void second_hidden_layer_process(int layer_num, int num_neurons,
                                 int read_fd, int write_fd, int total_hidden_layers) {
    // Is process ke liye output file kholo (append mode)
    FILE *local_result_file = fopen("output.txt", "a");
    if (!local_result_file) {
        fprintf(stderr, "ERROR: Cannot open output.txt\n");
        exit(1);
    }
    
    // Open input file for this process
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        fclose(local_result_file);
        exit(1);
    }
    
    // Skip: input values + input layer weights + all hidden layer weights + output layer weights
    // + second pass input layer weights + previous second pass hidden layer weights
    double dummy;
    parse_double_with_comma(input_fp, &dummy);
    parse_double_with_comma(input_fp, &dummy);
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    for (int i = 0; i < total_hidden_layers; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    for (int i = 0; i < num_neurons * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    // Skip second pass input layer weights (neurons_count weights for each of neurons_count inputs)
    for (int i = 0; i < num_neurons * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    // Skip previous second pass hidden layer weights
    for (int i = 0; i < layer_num - 1; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        fclose(input_fp);
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            fclose(input_fp);
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "FORWARD PASS 2 - LAYER %d OUTPUT\n", layer_num);
    fprintf(local_result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(local_result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(local_result_file, "\n");
    fflush(local_result_file);
    pthread_mutex_unlock(&file_lock);
    
    // Send output through pipe
    if (!write_to_pipe(write_fd, output, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write to pipe\n");
        exit(1);
    }
    
    close(write_fd);
    free(input_data);
    free(weights);
    free(output);
    fclose(input_fp);
    fclose(local_result_file);
    
    exit(0);
}

// Second forward pass - output layer
void second_output_layer_process(int layer_num, int num_neurons, int read_fd, int total_hidden_layers) {
    // Is process ke liye output file kholo (append mode)
    FILE *local_result_file = fopen("output.txt", "a");
    if (!local_result_file) {
        fprintf(stderr, "ERROR: Cannot open output.txt\n");
        exit(1);
    }
    
    // Open input file for this process
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        fclose(local_result_file);
        exit(1);
    }
    
    // Skip: input values + input layer weights + all hidden layer weights + output layer weights
    // + second pass input layer weights + all second pass hidden layer weights
    double dummy;
    parse_double_with_comma(input_fp, &dummy);
    parse_double_with_comma(input_fp, &dummy);
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    for (int i = 0; i < total_hidden_layers; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    for (int i = 0; i < num_neurons * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    // Skip second pass input layer weights
    for (int i = 0; i < num_neurons * num_neurons; i++) {
        parse_double_with_comma(input_fp, &dummy);
    }
    // Skip all second pass hidden layer weights
    for (int i = 0; i < total_hidden_layers; i++) {
        for (int j = 0; j < num_neurons * num_neurons; j++) {
            parse_double_with_comma(input_fp, &dummy);
        }
    }
    
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        fclose(input_fp);
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            fclose(input_fp);
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(local_result_file, "FORWARD PASS 2 - FINAL OUTPUT LAYER\n");
    fprintf(local_result_file, "Final Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(local_result_file, "  Output[%d] = %.6f\n", i, output[i]);
    }
    fprintf(local_result_file, "\n");
    fprintf(local_result_file, "SIMULATION COMPLETED SUCCESSFULLY\n");
    fflush(local_result_file);
    pthread_mutex_unlock(&file_lock);
    
    free(input_data);
    free(weights);
    free(output);
    fclose(input_fp);
    fclose(local_result_file);
    
    exit(0);
}

// Main function - program yahan se start hota hai
int main() {
    printf("\n");
    printf("*==================================================*\n");
    printf("*  NEURAL NETWORK MULTI-CORE SIMULATOR            *\n");
    printf("*  Process & Thread Based Architecture            *\n");
    printf("*==================================================*\n\n");
    
    // Input file check karo - file exist karti hai ya nahi
    if (!validate_file_exists("input.txt")) {
        exit(1);
    }
    
    // Files open karo - input read karne ke liye aur output write karne ke liye
    FILE *input_fp = fopen("input.txt", "r");
    // Main process opens output file in write mode (truncates file)
    result_file = fopen("output.txt", "w");
    
    if (!result_file) {
        fprintf(stderr, "ERROR: Cannot write to output.txt\n");
        fclose(input_fp);
        exit(1);
    }
    
    // User se configuration input lo - kitne hidden layers aur kitne neurons
    int layers_count, neurons_count;
    
    // User se configuration input lo
    printf("CONFIGURATION INPUT\n");
    printf("-------------------\n");
    printf("Number of hidden layers (valid range 1-9): ");
    fflush(stdout);
    
    // Hidden layers count input lo
    if (scanf("%d", &layers_count) != 1) {
        fprintf(stderr, "ERROR: Invalid hidden layers input\n");
        exit(1);
    }
    
    // Validation - range check
    if (layers_count < 1 || layers_count > 9) {
        fprintf(stderr, "ERROR: Hidden layers must be between 1 and 9\n");
        exit(1);
    }
    
    // Neurons per layer input lo
    printf("Neurons per layer (valid range 1-100): ");
    fflush(stdout);
    
    if (scanf("%d", &neurons_count) != 1) {
        fprintf(stderr, "ERROR: Invalid neurons input\n");
        exit(1);
    }
    
    // Validation - range check
    if (neurons_count < 1 || neurons_count > 100) {
        fprintf(stderr, "ERROR: Neurons must be between 1 and 100\n");
        exit(1);
    }
    
    printf("\n[STATUS] Configuration accepted.\n");
    printf("[STATUS] Starting simulation with %d hidden layers, %d neurons/layer\n\n", 
           layers_count, neurons_count);
    
    // Write header only once in main process (before fork)
    fprintf(result_file, "NEURAL NETWORK SIMULATION REPORT\n");
    fprintf(result_file, "=================================\n");
    fprintf(result_file, "Configuration: %d Hidden Layers | %d Neurons Per Layer\n\n", 
            layers_count, neurons_count);
    fflush(result_file);  // Ensure header is written before fork
    fclose(result_file);  // Close in main - children will open separately in append mode
    
    // ========== FORWARD PASS 1 ==========
    // Pehla forward pass - input se output tak
    
    // Forward pass ke liye pipes create karo (IPC channels)
    // Har layer ke beech mein ek pipe: input->hidden1, hidden1->hidden2, ..., hidden->output
    int forward_pipes[layers_count + 2][2];  // +2 kyunki input->first hidden aur last hidden->output
    for (int i = 0; i < layers_count + 2; i++) {
        if (pipe(forward_pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
    
    // Backward pass ke liye pipe create karo (output se input tak)
    int backward_pipe[2];
    if (pipe(backward_pipe) == -1) {
        perror("pipe");
        exit(1);
    }
    
    // Input layer process create karo (fork se)
    // fork() ek naya process create karta hai - yeh OS concept hai
    pid_t input_pid = fork();
    if (input_pid == 0) {
        // Child process - yeh input layer hai
        close(forward_pipes[0][0]);  // Read end close karo (hum sirf write karenge)
        input_layer_process(neurons_count, neurons_count, forward_pipes[0][1], 0);
    } else if (input_pid < 0) {
        perror("fork");  // Fork fail ho gaya
        exit(1);
    }
    // Parent process - write end close karo (child use karega)
    close(forward_pipes[0][1]);
    
    // Calculate file offsets: each process needs to know where to start reading
    // We'll pass 0 as offset and let each process calculate based on layer number
    // Actually simpler: calculate current file position for each layer
    
    // Hidden layer processes create karo - har hidden layer alag process hai
    pid_t hidden_pids[layers_count];
    for (int i = 0; i < layers_count; i++) {
        hidden_pids[i] = fork();  // Naya process create karo
        if (hidden_pids[i] == 0) {
            // Child process - yeh hidden layer hai
            close(forward_pipes[i][1]);      // Previous layer ka write end close
            close(forward_pipes[i + 1][0]);  // Next layer ka read end close
            hidden_layer_process(i + 1, neurons_count, 
                               forward_pipes[i][0], forward_pipes[i + 1][1], 
                               layers_count);
        } else if (hidden_pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        // Parent process - unused pipe ends close karo
        close(forward_pipes[i][0]);
        close(forward_pipes[i + 1][1]);
    }
    
    // Output layer process create karo
    pid_t output_pid = fork();
    if (output_pid == 0) {
        // Child process - yeh output layer hai
        close(forward_pipes[layers_count][1]);  // Write end close
        close(backward_pipe[0]);                // Backward pipe ka read end close
        output_layer_process(layers_count + 1, neurons_count, 
                           forward_pipes[layers_count][0], backward_pipe[1], 
                           layers_count);
    } else if (output_pid < 0) {
        perror("fork");
        exit(1);
    }
    // Parent process - unused ends close karo
    close(forward_pipes[layers_count][0]);
    close(backward_pipe[1]);
    
    // Sab processes ke complete hone ka wait karo (waitpid)
    // Yeh zaroori hai taake parent process sab child processes ke khatam hone ka wait kare
    waitpid(input_pid, NULL, 0);
    for (int i = 0; i < layers_count; i++) {
        waitpid(hidden_pids[i], NULL, 0);
    }
    waitpid(output_pid, NULL, 0);
    
    // ========== SECOND FORWARD PASS ==========
    // Doosra forward pass - backward outputs ko naye inputs ki tarah use karke
    
    printf("[PHASE] SECOND FORWARD PASS\n");
    printf("  Using backward outputs as new inputs...\n\n");
    
    // Second forward pass ke liye pipes create karo
    int second_forward_pipes[layers_count + 2][2];
    for (int i = 0; i < layers_count + 2; i++) {
        if (pipe(second_forward_pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
    
    // Fork second input layer process
    pid_t second_input_pid = fork();
    if (second_input_pid == 0) {
        close(backward_pipe[1]);  // Close write end
        close(second_forward_pipes[0][0]);  // Close read end
        second_input_layer_process(neurons_count, neurons_count,
                                  backward_pipe[0], second_forward_pipes[0][1], 
                                  layers_count);
    } else if (second_input_pid < 0) {
        perror("fork");
        exit(1);
    }
    close(backward_pipe[0]);
    close(second_forward_pipes[0][1]);
    
    // Fork second hidden layer processes
    pid_t second_hidden_pids[layers_count];
    for (int i = 0; i < layers_count; i++) {
        second_hidden_pids[i] = fork();
        if (second_hidden_pids[i] == 0) {
            close(second_forward_pipes[i][1]);
            close(second_forward_pipes[i + 1][0]);
            second_hidden_layer_process(i + 1, neurons_count,
                                       second_forward_pipes[i][0], 
                                       second_forward_pipes[i + 1][1],
                                       layers_count);
        } else if (second_hidden_pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        close(second_forward_pipes[i][0]);
        close(second_forward_pipes[i + 1][1]);
    }
    
    // Fork second output layer process
    pid_t second_output_pid = fork();
    if (second_output_pid == 0) {
        close(second_forward_pipes[layers_count][1]);
        second_output_layer_process(layers_count + 1, neurons_count,
                                   second_forward_pipes[layers_count][0], 
                                   layers_count);
    } else if (second_output_pid < 0) {
        perror("fork");
        exit(1);
    }
    close(second_forward_pipes[layers_count][0]);
    
    // Wait for all second forward pass processes
    waitpid(second_input_pid, NULL, 0);
    for (int i = 0; i < layers_count; i++) {
        waitpid(second_hidden_pids[i], NULL, 0);
    }
    waitpid(second_output_pid, NULL, 0);
    
    // Cleanup - sab resources free karo
    // Pipes close karo (unnamed pipes automatically cleanup ho jati hain)
    for (int i = 0; i < layers_count + 2; i++) {
        close(forward_pipes[i][0]);
        close(forward_pipes[i][1]);
        close(second_forward_pipes[i][0]);
        close(second_forward_pipes[i][1]);
    }
    close(backward_pipe[0]);
    close(backward_pipe[1]);
    
    // Files close karo
    fclose(input_fp);
    // result_file already closed earlier (before fork)
    // Mutex destroy karo
    pthread_mutex_destroy(&file_lock);
    
    printf("  Second forward pass complete\n\n");
    printf("*==================================================*\n");
    printf("* SIMULATION FINISHED\n");
    printf("* Results saved to output.txt\n");
    printf("*==================================================*\n\n");
    
    return 0;
}
