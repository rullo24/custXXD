# Custom xxd Clone in C
This project is a C-based implementation of the xxd command-line tool, designed to convert input data into a hexadecimal representation. The tool supports input via the command line or through standard input (piped input) while ensuring safe and efficient memory management practices.

## Features
- Piped Input Handling: Reads data from standard input, making it suitable for use with other shell utilities.
- Argument Parsing: Handles file input passed as a command-line argument, with error checking to ensure only one argument is provided.
- Memory Safety:
    - Allocates and zeroes memory to prevent undefined behavior.
    - Implements a robust cleanup routine to free allocated resources and prevent memory leaks.
    - Tested with valgrind to ensure no memory leaks or invalid memory access.

## Usage
### Reading Data via Argument (argv)
To convert an argument's context to a hex dump:
```bash
./custXXD *string*
```
NOTE: No quotation marks are required.

### Reading Data via Pipe (|)
```bash
echo "*string*" | ./custXXD
```