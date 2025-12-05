#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_NEURONS 100
#define INPUT_NEURONS 2
#define BUFFER_SIZE 8192

typedef struct {
    int thread_id;
    int input_size;
    double *layer_inputs;
    double *neuron_weights;
    double *output_array;
    pthread_mutex_t *sync_lock;
} ComputeThread;

// Global output file with mutex
FILE *result_file;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

// Function to parse input file with comma handling
int parse_double_with_comma(FILE *fp, double *value) {
    char buffer[256];
    char *line = NULL;
    size_t len = 0;
    
    // Read until we get a valid number
    while (1) {
        int c = fgetc(fp);
        if (c == EOF) return 0;
        
        // Skip whitespace
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        
        // If we hit a comma, skip it
        if (c == ',') continue;
        
        // Put back the character and read the number
        ungetc(c, fp);
        if (fscanf(fp, "%lf", value) == 1) {
            // Skip any trailing comma
            c = fgetc(fp);
            if (c != ',') ungetc(c, fp);
            return 1;
        }
        return 0;
    }
}

int validate_file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: Input file '%s' not found or cannot be read\n", path);
        return 0;
    }
    fclose(f);
    return 1;
}

void *execute_neuron_task(void *params) {
    ComputeThread *task = (ComputeThread *)params;
    double sum = 0.0;
    
    for (int j = 0; j < task->input_size; j++) {
        sum += task->layer_inputs[j] * task->neuron_weights[j];
    }
    
    pthread_mutex_lock(task->sync_lock);
    task->output_array[task->thread_id] = sum;
    pthread_mutex_unlock(task->sync_lock);
    
    pthread_exit(NULL);
}

double* launch_neuron_threads(int num_neurons, int input_size, 
                              double *input_data, double *weights) {
    double *results = (double *)malloc(num_neurons * sizeof(double));
    if (!results) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(1);
    }
    
    pthread_t tid_array[num_neurons];
    pthread_mutex_t compute_lock = PTHREAD_MUTEX_INITIALIZER;
    ComputeThread task_params[num_neurons];
    
    for (int i = 0; i < num_neurons; i++) {
        task_params[i].thread_id = i;
        task_params[i].input_size = input_size;
        task_params[i].layer_inputs = input_data;
        task_params[i].neuron_weights = &weights[i * input_size];
        task_params[i].output_array = results;
        task_params[i].sync_lock = &compute_lock;
        
        pthread_create(&tid_array[i], NULL, execute_neuron_task, &task_params[i]);
    }
    
    for (int i = 0; i < num_neurons; i++) {
        pthread_join(tid_array[i], NULL);
    }
    
    pthread_mutex_destroy(&compute_lock);
    return results;
}

// Write array to pipe
int write_to_pipe(int pipe_fd, double *data, int count) {
    // Write count first
    if (write(pipe_fd, &count, sizeof(int)) != sizeof(int)) {
        return 0;
    }
    // Write data
    if (write(pipe_fd, data, count * sizeof(double)) != count * sizeof(double)) {
        return 0;
    }
    return 1;
}

// Read array from pipe
int read_from_pipe(int pipe_fd, double **data, int *count) {
    int n;
    if (read(pipe_fd, &n, sizeof(int)) != sizeof(int)) {
        return 0;
    }
    
    *count = n;
    *data = (double *)malloc(n * sizeof(double));
    if (!*data) {
        return 0;
    }
    
    if (read(pipe_fd, *data, n * sizeof(double)) != n * sizeof(double)) {
        free(*data);
        return 0;
    }
    return 1;
}

// Input layer process
void input_layer_process(int num_neurons, int neurons_per_layer, 
                        int write_fd, int layer_id) {
    printf("[LAYER %d] INPUT LAYER (PID: %d)\n", layer_id, getpid());
    printf("  Input neurons: %d\n", INPUT_NEURONS);
    
    // Open input file for this process
    FILE *input_fp = fopen("input.txt", "r");
    if (!input_fp) {
        fprintf(stderr, "ERROR: Cannot open input.txt\n");
        exit(1);
    }
    
    // Read input values
    double input_values[INPUT_NEURONS];
    if (!parse_double_with_comma(input_fp, &input_values[0]) ||
        !parse_double_with_comma(input_fp, &input_values[1])) {
        fprintf(stderr, "ERROR: Failed to read initial input values\n");
        fclose(input_fp);
        exit(1);
    }
    
    printf("  Values: [%.4f, %.4f]\n", input_values[0], input_values[1]);
    
    // Read weights
    double *weights = (double *)malloc(INPUT_NEURONS * num_neurons * sizeof(double));
    for (int i = 0; i < INPUT_NEURONS * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, INPUT_NEURONS, 
                                           input_values, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "FORWARD PASS 1 - INPUT LAYER COMPUTATION\n");
    fprintf(result_file, "Input: [%.6f, %.6f]\n", input_values[0], input_values[1]);
    fprintf(result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(result_file, "\n");
    pthread_mutex_unlock(&file_lock);
    
    // Send output through pipe
    if (!write_to_pipe(write_fd, output, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write to pipe\n");
        exit(1);
    }
    
    close(write_fd);
    free(weights);
    free(output);
    fclose(input_fp);
    
    printf("  Output sent to next layer (processing complete)\n\n");
    exit(0);
}

// Hidden layer process
void hidden_layer_process(int layer_num, int num_neurons, 
                         int read_fd, int write_fd, FILE *input_fp) {
    printf("[LAYER %d] HIDDEN LAYER (PID: %d)\n", layer_num, getpid());
    printf("  Neurons: %d\n", num_neurons);
    
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "FORWARD PASS 1 - HIDDEN LAYER %d COMPUTATION\n", layer_num);
    fprintf(result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(result_file, "\n");
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
    
    printf("  Processing complete\n\n");
    exit(0);
}

// Output layer process
void output_layer_process(int layer_num, int num_neurons, 
                         int read_fd, int backward_write_fd, FILE *input_fp) {
    printf("[LAYER %d] OUTPUT LAYER (PID: %d)\n", layer_num, getpid());
    printf("  Neurons: %d\n", num_neurons);
    
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "FORWARD PASS 1 - OUTPUT LAYER COMPUTATION\n");
    fprintf(result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(result_file, "  Output[%d] = %.6f\n", i, output[i]);
    }
    fprintf(result_file, "\n");
    pthread_mutex_unlock(&file_lock);
    
    printf("  Processing complete\n\n");
    
    // Backward pass computation
    printf("[PHASE] BACKWARD PROPAGATION (PID: %d)\n", getpid());
    printf("  Computing activation functions...\n\n");
    
    double *backward_data = (double *)malloc(num_neurons * sizeof(double));
    if (!backward_data) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(1);
    }
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "BACKWARD PASS COMPUTATION\n");
    fprintf(result_file, "Formula 1: f(x1) = (x^2 + x + 1) / 2\n");
    fprintf(result_file, "Formula 2: f(x2) = (x^2 - x) / 2\n");
    fprintf(result_file, "Results:\n");
    
    for (int i = 0; i < num_neurons; i++) {
        double val = output[i];
        double fx1 = ((val * val) + val + 1.0) / 2.0;
        double fx2 = ((val * val) - val) / 2.0;
        
        backward_data[i] = fx1;  // Use f(x1) for backward pass
        fprintf(result_file, "  Neuron[%d]: f(x1)=%.6f | f(x2)=%.6f\n", i, fx1, fx2);
    }
    fprintf(result_file, "\n");
    pthread_mutex_unlock(&file_lock);
    
    // Send backward data through pipe
    if (!write_to_pipe(backward_write_fd, backward_data, num_neurons)) {
        fprintf(stderr, "ERROR: Failed to write backward data\n");
        exit(1);
    }
    close(backward_write_fd);
    
    free(input_data);
    free(weights);
    free(output);
    free(backward_data);
    
    printf("  Backward computation complete\n\n");
    exit(0);
}

// Second forward pass - input layer
void second_input_layer_process(int num_neurons, int neurons_per_layer,
                                int read_backward_fd, int write_fd, FILE *input_fp) {
    printf("[PHASE] SECOND FORWARD PASS - INPUT LAYER (PID: %d)\n", getpid());
    printf("  Using backward outputs as new inputs...\n\n");
    
    // Read backward data
    double *backward_data;
    int backward_count;
    if (!read_from_pipe(read_backward_fd, &backward_data, &backward_count)) {
        fprintf(stderr, "ERROR: Failed to read backward data\n");
        exit(1);
    }
    close(read_backward_fd);
    
    // Read weights for second pass
    double *weights = (double *)malloc(backward_count * num_neurons * sizeof(double));
    for (int i = 0; i < backward_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data for second pass\n");
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, backward_count, 
                                           backward_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "FORWARD PASS 2 - LAYER 1 OUTPUT\n");
    fprintf(result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(result_file, "\n");
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
    
    exit(0);
}

// Second forward pass - hidden layer
void second_hidden_layer_process(int layer_num, int num_neurons,
                                 int read_fd, int write_fd, FILE *input_fp) {
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "FORWARD PASS 2 - LAYER %d OUTPUT\n", layer_num);
    fprintf(result_file, "Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(result_file, "  Neuron[%d] = %.6f\n", i, output[i]);
    }
    fprintf(result_file, "\n");
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
    
    exit(0);
}

// Second forward pass - output layer
void second_output_layer_process(int layer_num, int num_neurons, int read_fd, FILE *input_fp) {
    // Read input from previous layer
    double *input_data;
    int input_count;
    if (!read_from_pipe(read_fd, &input_data, &input_count)) {
        fprintf(stderr, "ERROR: Failed to read from pipe\n");
        exit(1);
    }
    close(read_fd);
    
    // Read weights
    double *weights = (double *)malloc(input_count * num_neurons * sizeof(double));
    for (int i = 0; i < input_count * num_neurons; i++) {
        if (!parse_double_with_comma(input_fp, &weights[i])) {
            fprintf(stderr, "ERROR: Insufficient weight data\n");
            exit(1);
        }
    }
    
    // Process with threads
    double *output = launch_neuron_threads(num_neurons, input_count, 
                                           input_data, weights);
    
    pthread_mutex_lock(&file_lock);
    fprintf(result_file, "FORWARD PASS 2 - FINAL OUTPUT LAYER\n");
    fprintf(result_file, "Final Output:\n");
    for (int i = 0; i < num_neurons; i++) {
        fprintf(result_file, "  Output[%d] = %.6f\n", i, output[i]);
    }
    fprintf(result_file, "\n");
    fprintf(result_file, "SIMULATION COMPLETED SUCCESSFULLY\n");
    pthread_mutex_unlock(&file_lock);
    
    free(input_data);
    free(weights);
    free(output);
    
    exit(0);
}

int main() {
    printf("\n");
    printf("*==================================================*\n");
    printf("*  NEURAL NETWORK MULTI-CORE SIMULATOR            *\n");
    printf("*  Process & Thread Based Architecture            *\n");
    printf("*==================================================*\n\n");
    
    // Input validation
    if (!validate_file_exists("input.txt")) {
        exit(1);
    }
    
    FILE *input_fp = fopen("input.txt", "r");
    result_file = fopen("output.txt", "w");
    
    if (!result_file) {
        fprintf(stderr, "ERROR: Cannot write to output.txt\n");
        fclose(input_fp);
        exit(1);
    }
    
    // Configuration input
    int layers_count, neurons_count;
    
    printf("CONFIGURATION INPUT\n");
    printf("-------------------\n");
    printf("Number of hidden layers (valid range 1-9): ");
    fflush(stdout);
    
    if (scanf("%d", &layers_count) != 1) {
        fprintf(stderr, "ERROR: Invalid hidden layers input\n");
        exit(1);
    }
    
    if (layers_count < 1 || layers_count > 9) {
        fprintf(stderr, "ERROR: Hidden layers must be between 1 and 9\n");
        exit(1);
    }
    
    printf("Neurons per layer (valid range 1-100): ");
    fflush(stdout);
    
    if (scanf("%d", &neurons_count) != 1) {
        fprintf(stderr, "ERROR: Invalid neurons input\n");
        exit(1);
    }
    
    if (neurons_count < 1 || neurons_count > 100) {
        fprintf(stderr, "ERROR: Neurons must be between 1 and 100\n");
        exit(1);
    }
    
    printf("\n[STATUS] Configuration accepted.\n");
    printf("[STATUS] Starting simulation with %d hidden layers, %d neurons/layer\n\n", 
           layers_count, neurons_count);
    
    fprintf(result_file, "NEURAL NETWORK SIMULATION REPORT\n");
    fprintf(result_file, "=================================\n");
    fprintf(result_file, "Configuration: %d Hidden Layers | %d Neurons Per Layer\n\n", 
            layers_count, neurons_count);
    
    // ========== FORWARD PASS 1 ==========
    
    // Create pipes for forward pass
    int forward_pipes[layers_count + 2][2];  // +2 for input->first hidden and last hidden->output
    for (int i = 0; i < layers_count + 2; i++) {
        if (pipe(forward_pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
    
    // Create pipe for backward pass
    int backward_pipe[2];
    if (pipe(backward_pipe) == -1) {
        perror("pipe");
        exit(1);
    }
    
    // Fork input layer process
    pid_t input_pid = fork();
    if (input_pid == 0) {
        close(forward_pipes[0][0]);  // Close read end
        input_layer_process(neurons_count, neurons_count, forward_pipes[0][1], 
                           input_fp, 0);
    } else if (input_pid < 0) {
        perror("fork");
        exit(1);
    }
    close(forward_pipes[0][1]);
    
    // Fork hidden layer processes
    pid_t hidden_pids[layers_count];
    for (int i = 0; i < layers_count; i++) {
        hidden_pids[i] = fork();
        if (hidden_pids[i] == 0) {
            close(forward_pipes[i][1]);  // Close write end of previous
            close(forward_pipes[i + 1][0]);  // Close read end of next
            hidden_layer_process(i + 1, neurons_count, 
                               forward_pipes[i][0], forward_pipes[i + 1][1], 
                               input_fp);
        } else if (hidden_pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        close(forward_pipes[i][0]);
        close(forward_pipes[i + 1][1]);
    }
    
    // Fork output layer process
    pid_t output_pid = fork();
    if (output_pid == 0) {
        close(forward_pipes[layers_count][1]);  // Close write end
        close(backward_pipe[0]);  // Close read end of backward pipe
        output_layer_process(layers_count + 1, neurons_count, 
                           forward_pipes[layers_count][0], backward_pipe[1], 
                           input_fp);
    } else if (output_pid < 0) {
        perror("fork");
        exit(1);
    }
    close(forward_pipes[layers_count][0]);
    close(backward_pipe[1]);
    
    // Wait for all forward pass processes
    waitpid(input_pid, NULL, 0);
    for (int i = 0; i < layers_count; i++) {
        waitpid(hidden_pids[i], NULL, 0);
    }
    waitpid(output_pid, NULL, 0);
    
    // ========== SECOND FORWARD PASS ==========
    
    printf("[PHASE] SECOND FORWARD PASS\n");
    printf("  Using backward outputs as new inputs...\n\n");
    
    // Create pipes for second forward pass
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
                                  input_fp);
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
                                       input_fp);
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
                                   input_fp);
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
    
    // Cleanup
    for (int i = 0; i < layers_count + 2; i++) {
        close(forward_pipes[i][0]);
        close(forward_pipes[i][1]);
        close(second_forward_pipes[i][0]);
        close(second_forward_pipes[i][1]);
    }
    close(backward_pipe[0]);
    close(backward_pipe[1]);
    
    fclose(input_fp);
    fclose(result_file);
    pthread_mutex_destroy(&file_lock);
    
    printf("  Second forward pass complete\n\n");
    printf("*==================================================*\n");
    printf("* SIMULATION FINISHED\n");
    printf("* Results saved to output.txt\n");
    printf("*==================================================*\n\n");
    
    return 0;
}
