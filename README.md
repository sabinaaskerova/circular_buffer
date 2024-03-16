### Multithreaded Program with Circular Buffer

This repository contains a multithreaded program implemented in C based on POSIX barriers and conditions. The program takes the following command-line arguments:

- Number of threads to create.
- Size of a circular buffer, specified in the number of slots.
- An optional sequence of non-negative values.

The main thread creates the circular buffer and child threads, then transmits the values to the child threads via the buffer. Each thread extracts a value from the buffer at each iteration and waits (using `sleep`) for the number of seconds specified in the buffer. Once the main thread has transmitted all the values, it writes as many "-1" values into the buffer as there are threads to signal them to stop. After all threads have finished processing the received values, each thread prints the character "*" and terminates.

#### Program Features:
- **Dynamic Thread Creation**: The program dynamically creates threads based on the user-provided number of threads.
- **Circular Buffer**: Utilizes a circular buffer for inter-thread communication.
- **Thread Synchronization**: Uses POSIX mutexes, conditions, and barriers for thread synchronization without active waiting.
- **Graceful Termination**: Ensures all threads terminate gracefully after completing their tasks.
- **Input Flexibility**: Accepts an optional sequence of non-negative values to transmit to the threads.

#### Usage
```bash
./a.out <number_of_threads> <buffer_size> <value_1> ... <value_n>
```

- `<number_of_threads>`: Number of threads to create (must be greater than 0).
- `<buffer_size>`: Size of the circular buffer (must be greater than 0).
- `<value_1> ... <value_n>`: Optional sequence of non-negative values to transmit to the threads.

#### Example
```bash
./a.out 2 4 5 3 7 1
```

This command creates 2 threads and a circular buffer of size 4. It then transmits the values 5, 3, 7, and 1 to the threads.

#### Compilation
```bash
gcc -Wall -Wextra -Werror -g -pthread circ_buff.c -o a.out
```

#### Side notes
- No global variables are used.
- No active waiting is implemented.
- No concurrency issues are present.
- Calls to `sleep` are parallelizable by different threads.
- The "*" character is displayed only after all threads have exhausted the transmitted values and finished their corresponding waits.
- Only POSIX barriers and conditions are used for synchronization.

#### Notions learned

Multithreading: The program demonstrates how to create and manage multiple threads using POSIX threads (pthread) library in C.

Synchronization: It shows how to synchronize threads using POSIX synchronization primitives such as mutexes (pthread_mutex_t), condition variables (pthread_cond_t), and barriers (pthread_barrier_t).

Circular Buffer: The program utilizes a circular buffer data structure to facilitate communication between the main thread and the child threads.

POSIX Compliance: The program strictly adheres to POSIX standards for synchronization and thread management.

Concurrency: The program demonstrates concurrent execution of multiple threads, teaching about concurrency issues such as race conditions, deadlocks, and thread synchronization.
