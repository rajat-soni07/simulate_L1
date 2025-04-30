# L1 Cache Simulator with MESI Protocol

This project simulates an L1 cache using the MESI protocol, implemented in C++.

## Compilation Instructions

Follow these steps to compile the code:

1. Open a terminal or command prompt.
2. Navigate to the directory containing the source files.
3. Execute the following command:
    ```bash
    make
    ```
4. The compilation process will generate an executable file named `l1_cache`.

## Usage instructions
```
./l1_cache [options]

-t <tracefile>: name of parallel application
-s <s>: number of set index bits
-E <E>: associativity (number of cache lines per set)
-b <b>: number of block bits
-o <outfilename>: logs output to file
-h: prints this help
```

## Team
- Rajat Soni (2023CS10229)
- Krish Bhimani (2023CS10712)