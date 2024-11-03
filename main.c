// Global #includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linux-specific #includes
#include <unistd.h>
#include <sys/ioctl.h>

// Macros defined for readability exclusively
#define CHAR_TO_HEX_MODIFIER 2
#define NULL_BYTE_SIZE sizeof(char)

// Pre-definition of necessary functions
int conv_str_to_hex_str(char **pp_malloc_str_hex_mem, size_t *p_hex_len_tracker, char *p_incoming_str_heap, size_t str_bytes_avail, size_t final_hex_max_size);
void cleanup(char **pp_malloc_str_hex_mem, char **pp_incoming_str_heap);

int main(int argc, char *argv[]) {
    // init vars that are use-specific --> to be checked before later use
    char *p_incoming_str_heap = NULL; // grabbing the only argument (excl .exe) --> handle when multiple args parsed
    size_t str_bytes_avail = 0; // captures the size of the incoming string
    char *p_malloc_str_hex_mem = NULL; // init here to allow for easy cleanup func

    // reacting to piped string
    if (argc == 1) { 
        int stdin_bytes_avail = 0; // used to store the num of bytes available in pipe

        // using ioctl to communicate w/ the stdin file descriptor
        int ioctl_result = ioctl(STDIN_FILENO, FIONREAD, &stdin_bytes_avail); // will enter the num of bytes into stdin_bytes_avail if successful
        if (ioctl_result != 0) {
            fprintf(stderr, "ERROR: failed to get stdin descriptor using ioctl()\n");
            cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
            return -1;
        }
        str_bytes_avail = (size_t)stdin_bytes_avail; // calc num of bytes to alloc below
        if (str_bytes_avail == 0) {
            fprintf(stderr, "ERROR: no hex data available for processing\n");
            cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
            return -1;
        }

        // allocating memory for the stdin pipe input data
        size_t str_heap_size = sizeof(char) * str_bytes_avail + NULL_BYTE_SIZE;
        p_incoming_str_heap = (char*)malloc(str_heap_size); // don't need to add space for '\0' as '\n' is already included in byte size
        if (p_incoming_str_heap == NULL) {
            fprintf(stderr, "ERROR: failed to malloc for the heap str (using pipes)\n");
            cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
            return -1;
        }

        // init all allocated memory to zero (to avoid undefined behaviour)
        memset(p_incoming_str_heap, 0x0, str_heap_size); 

        // reading in the str from stdin (pipe)
        int stdin_bytes_read = read(STDIN_FILENO, p_incoming_str_heap, str_bytes_avail-NULL_BYTE_SIZE);
        if (stdin_bytes_read < 0) { // reacting to failed read
            fprintf(stderr, "ERROR: failed to read the piped input\n");
            cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
            return -1;
        }
        p_incoming_str_heap[str_bytes_avail-1] = '\0'; // switch read EOL char ('\n') for NULL byte ('\0') --> -1 to ensure still within bounds of buffer
         
    } else { // reacting to string parsed directly in argv
        if (argc > 2) { // checking if multiple strings have been parsed
            fprintf(stderr, "INVALID: More than 1x argument provided.\n");
            cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
            return -1;
        }

        // grabbing string and allocating memory for hex string
        char *p_incoming_str_stack = NULL; // grabbing the only argument (excl .exe) --> handle when multiple args parsed
        p_incoming_str_stack = argv[1]; // grabbing the only argument (excl .exe)
        str_bytes_avail = strlen(p_incoming_str_stack)+1;

        // allocating to heap for program simplicity across the two potential input methods
        size_t str_heap_size = sizeof(char) * str_bytes_avail + NULL_BYTE_SIZE;
        p_incoming_str_heap = (char*)malloc(str_heap_size); 
        if (p_incoming_str_heap == NULL) {
            fprintf(stderr, "ERROR: failed to malloc for the heap string (using args)\n");
            cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
            return -1;
        }

        // init all allocated memory to zero (to avoid undefined behaviour)
        memset(p_incoming_str_heap, 0x0, str_heap_size); 

        // copying the string from the stack ptr to the heap ptr
        memcpy(p_incoming_str_heap, p_incoming_str_stack, str_bytes_avail); 
        p_incoming_str_heap[str_bytes_avail-1] = '\0';
    }

    // checking for invalid parameters before continue
    if (p_incoming_str_heap == NULL) {
        fprintf(stderr, "ERROR: invalid init p_incoming_str_heap parameter detected\n");
        cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
        return -1;
    }
    if (str_bytes_avail == 0x0) {
        fprintf(stderr, "ERROR: invalid init str_bytes_avail parameter detected\n");
        cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
        return -1;
    }

    // calculating the required size of the hex str buffer
    size_t hex_str_size_wo_spaces_or_nb = sizeof(char) * str_bytes_avail * CHAR_TO_HEX_MODIFIER; 
    size_t req_num_spaces = hex_str_size_wo_spaces_or_nb / 4 + 1; // performing integer (truncate) division | adding 1 for extra space at start (to avoid issues with space counting) --> removed at end of program
    size_t final_hex_max_size = hex_str_size_wo_spaces_or_nb + req_num_spaces + NULL_BYTE_SIZE; // space after every 4x hex digits + space for \0

    // allocating memory on the heap (this could be implemented on the stack I have now realised)
    p_malloc_str_hex_mem = (char*)malloc(final_hex_max_size); // used for new hex string
    if (p_malloc_str_hex_mem == NULL) {
        fprintf(stderr, "ERROR: failed to allocate heap memory for hex conversion.\n");
        cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap);
        return -1;
    }
    
    // init all memory to zeros to avoid undefined behaviour
    memset(p_malloc_str_hex_mem, 0x0, final_hex_max_size);

    // converting the incoming string to a hex string --> checking for out-of-bounds
    size_t hex_len_tracker = 0; // var to capture the num of chars through string
    int str_to_hex_err = conv_str_to_hex_str(&p_malloc_str_hex_mem, &hex_len_tracker, p_incoming_str_heap, str_bytes_avail, final_hex_max_size);
    if (str_to_hex_err != 0) {
        fprintf(stderr, "ERROR: attempted to add chars outside of available buffer\n");
        cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap); // freeing heap allocated memory 
        return -1;
    }

    // pointer arithmetic to only include data after initial space --> simplest way of removing init space
    char *p_final_str_hex = p_malloc_str_hex_mem + sizeof(char); 
    printf("%s\n", p_final_str_hex);

    // freeing alloc'd mem
    cleanup(&p_malloc_str_hex_mem, &p_incoming_str_heap); // freeing heap allocated memory 
    return 0;
}

// func for converting a string of chars to a hex string
int conv_str_to_hex_str(char **pp_malloc_str_hex_mem, size_t *p_hex_len_tracker, char *p_incoming_str_heap, size_t str_bytes_avail, size_t final_hex_max_size) {
    // iterating over each letter in the parsed string
    int final_index = str_bytes_avail - NULL_BYTE_SIZE; // final index is smaller than num of str_bytes_avail due to zero indexing
    for (int letter_index=0; letter_index < final_index; letter_index++) { // strlen used as we know there is a \0 at end of string
        // init necessary vars
        char curr_letter = p_incoming_str_heap[letter_index];
        char letter_hex_string[2];

        // converting char to string that holds hex values (2)
        sprintf(letter_hex_string, "%02X", (unsigned char)curr_letter); 

        // adding current letter hex characters to the final hex string
        size_t hex_index = 0;
        while(letter_hex_string[hex_index] != '\0') {
            if (*p_hex_len_tracker < final_hex_max_size) { // adding new letter when space avail

                int space_freq = 5; // adding a space after every 4 hex chars
                if (*p_hex_len_tracker % space_freq == 0) { // adding space if successful
                    (*pp_malloc_str_hex_mem)[*p_hex_len_tracker] = ' ';
                } else {
                    (*pp_malloc_str_hex_mem)[*p_hex_len_tracker] = letter_hex_string[hex_index]; // adding each hex character to the final string
                    hex_index++; // moving to next char in the current letter hex string
                }
                *p_hex_len_tracker += 1; // incrementing the stored length of the final hex string

            } else { // ending string as we are now heading out of bounds
                return -1;
                break;
            }
        }
    }
    return 0;
}

// simplistic function for memory frees --> minimising code clutter in main func
void cleanup(char **pp_malloc_str_hex_mem, char **pp_incoming_str_heap) {
    // only free memory if it has been alloc'd
    if (*pp_malloc_str_hex_mem != NULL) {
        free(*pp_malloc_str_hex_mem);
        *pp_malloc_str_hex_mem = NULL; // NULLing to prevent dangling pointer use
    }

    if (*pp_incoming_str_heap != NULL) {
        free(*pp_incoming_str_heap);
        *pp_incoming_str_heap = NULL; // NULLing to prevent dangling pointer use
    }
}