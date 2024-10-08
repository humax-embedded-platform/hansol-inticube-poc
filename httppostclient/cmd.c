#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cmd.h"
#include "config.h"

static void cmd_print_usage(char* program_name) {
    printf("Usage: %s --request <number_of_requests> --input <input message> [--host <host_file>] [--log <log_folder>] [--help]\n", program_name);
    printf("  --request:  Number of requests to process (mandatory)\n");
    printf("  --input:    Input file send to host (mandatory)\n");
    printf("  --host:     Path to the host file (mandatory)\n");
    printf("  --log:      Path to the log folder (optional)\n");
    printf("  --help:     Display this help message\n");
}

static bool is_valid_integer(const char *str) {
    if (str == NULL || *str == '\0') {
        return false;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i])) {
            return false;
        }
    }

    return true;
}

bool is_txt_file(const char *filename) {  
    // Find the last occurrence of '.'  
    const char *dot = strrchr(filename, '.');  
    
    // If there is no dot or it's the first character, it's not a .txt file  
    if (!dot || dot == filename) {  
        return false; // Not a .txt file  
    }  

    // Compare the substring after the last dot with "txt"  
    return strcmp(dot + 1, "txt") == 0 ? true : false;  
}

int cmd_parser(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Error: --request, --host and --input are mandatory.\n");
        cmd_print_usage(argv[0]);
        return false;
    }

    bool request_found = false;
    bool input_found = false;
    bool host_found = false;
    bool log_found = false;
    int ret = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            cmd_print_usage(argv[0]);
            return 0;  // No error, just showing the help message
        } else if (strcmp(argv[i], "--request") == 0) {
            if ((i + 1) < argc) {
                const char *request_value = argv[++i];

                if (is_valid_integer(request_value)) {
                    int request_count = atoi(request_value);
                    if (config_set_request_count(request_count) < 0) {
                        printf("Error: Failed to set request count.\n");
                        return -1;
                    }
                    request_found = true;
                } else {
                    printf("Error: Invalid value for --request. Please enter a valid integer.\n");
                    return -1;
                }
            } else {
                printf("Error: Missing value for --request.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--input") == 0) {
            if ((i + 1) < argc) {
                const char *input_value = argv[++i];
                if (is_txt_file(input_value)) {
                    if (config_set_msg_file(input_value) < 0) {
                        printf("Error: Failed to set input file: %s.\n", argv[i]);
                        return -1;
                    }
                    input_found = true;
                } else {
                    printf("Error: Invalid value for --input. Please enter a .txt file.\n");
                    return -1;
                }
            } else {
                printf("Error: Missing value for --input.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--host") == 0) {
            if ((i + 1) < argc) {
                if (config_set_host_file(argv[++i]) < 0) {
                    printf("Error: Failed to set host file: %s.\n", argv[i]);
                    return -1;
                }
                host_found = true;
            } else {
                printf("Error: Missing value for --host.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--log") == 0) {
            if ((i + 1) < argc) {
                if (config_set_log_folder(argv[++i]) < 0) {
                    printf("Error: Failed to set log folder: %s.\n", argv[i]);
                    return -1;
                }
                log_found = true;
            } else {
                printf("Error: Missing value for --log.\n");
                return -1;
            }
        } else {
            printf("Error: Invalid option or missing value: %s.\n", argv[i]);
            cmd_print_usage(argv[0]);
            return -1;
        }
    }

    if (!request_found) {
        printf("Error: Missing --request option.\n");
        cmd_print_usage(argv[0]);
        return -1;
    }

    if (!input_found) {
        printf("Error: Missing --input option.\n");
        cmd_print_usage(argv[0]);
        return -1;
    }

    if (!host_found) {
        printf("Error: Missing --host option.\n");
        cmd_print_usage(argv[0]);
        return -1;
    }

    return 0;
}
