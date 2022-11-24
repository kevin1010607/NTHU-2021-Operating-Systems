# NTHU 2021 Operating Systems

## MP1 - System Call

### Goal
- Understand how to work in Linux environment.
- Understand how system calls are implemented by OS.
- Understand the difference between user mode and kernel mode.
### Implementation
- Implement four I/O system calls in NachOS.
  - `OpenFileId Open(char *name);`
  - `int Write(char *buffer, int size, OpenFileId id);`
  - `int Read(char *buffer, int size, OpenFileId id);`
  - `int Close(OpenFileId id);`

## MP2 - Multi-Programming

### Goal
- Understand how memory management works in NachOS.
- Understand how to implement page table mechanism.
### Implementation
- Implement page table in NachOS.

## MP3 - CPU scheduling

### Goal
- The default CPU scheduling algorithm of Nachos is a simple round-robin scheduler with 100 ticks time quantum.
- The goal of this MP is to replace it with a multilevel feedback queue, and understand the implementation of process lifecycle management and context switch mechanism.
### Implementation
- Implement a multilevel feedback queue scheduler with aging mechanism.
- Add a command line argument `-ep` for nachos to initialize priority of process
- Add a debugging flag `z` and use the `DEBUG('z', expr)` macro (defined in *debug.h*) to print messages.

## MP4 - File System

### Goal
- NachOS native file system (NachOS FS) only supports up to 4KB file size and only has a root directory. In this assignment, you need to study NachOS FS and find out the reason for limitations.
- Also, you are required to enhance the file system to let NachOS support larger file size and subdirectory structures.
### Implementation
- Implement five system calls to to support file I/O system call.
  - `int Create(char *name, int size);`
  - `OpenFileId Open(char *name);`
  - `int Read(char *buf, int size, OpenFileId id);`
  - `int Write(char *buf, int size, OpenFileId id);`
  - `int Close(OpenFileId id);`
- Enhance the FS to let it support up to 32KB file size
- Implement the subdirectory structure.
- Support up to 64 files/subdirectories per directory.
### Bonus
- Enhance the NachOS to support even larger file size.
  - Extend the disk from 128KB to 64MB.
  - Support up to 64 MB single file.
- Multi-level header size.
  - Show that smaller file can have smaller header size.
  - Implement at least 3 different size of headers for different size of files.
  - Design your own test cases to show your implementation is correct.
- Recursive operations on directories.
  - Support recursive remove of a directory.

## Pthread - Producer-Consumer Problem

### Goal
- Understand how to work in the Linux environment.
- Understand how multi-thread programming can be done with the pthread library.
### Implementation
- TSQueue: Implement a thread safe queue to support enqueue, dequeue, getsize operations.
- Producer: Create 4 Producer threads on the program starts. When each Producer thread starts, it repeats to:
  - Take an Item from Input Queue.
  - Use Transformer::producer_transform to perform transform on the Item’s value.
  - Put the Item with new value into the Worker Queue.
  - Do not modify the Item’s key and the Item’s opcode.
- Consumer: When the Consumer thread starts, it repeats to:
  - Take an Item from the Worker Queue.
  - Use Transformer::consumer_transform to perform transform on the Item’s value.
  - Put the Item with new value into the Output Queue.
  - Do not modify the Item’s key and the Item’s opcode.
- ConsumerController: Create a ConsumerController when the program starts. When the ConsumerController thread starts, is repeats to:
  - In the beginning, no Consumer will be created by ConsumerController.
  - Check Work Queue status periodically. (The period is defined in main.cpp as CONSUMER_CONTROLLER_CHECK_PERIOD).
    - When the Worker Queue size exceeds *high_threshold*, create a new Consumer thread.
    - When the Worker Queue size falls behind a *low_threshold*, cancel the newest Consumer (by calling Consumer->cancel).
    - Once you have created a Consumer, make sure there is at least one Consumer until the program ends.
    - Maintains the list of running Consumers in ConsumerController::consumers.
- Writer: Create a Writer when the program starts. On the Writer thread starts, it repeats to:
  - Take an Item from the Output Queue.
  - Write the Item to output file with the same format as the input file ({key} {value} {opcode}).
- main.cpp: Implement your main function, the main function ends after reader thread and writer thread to complete.