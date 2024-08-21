#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"
#include "config.h"

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 128

static void cmd_print_usage(char* program_name) {
    printf("Usage: %s --request --input <input message> <number_of_requests> [--host <host_file>] [--log <log_folder>]\n", program_name);
    printf("  --request:  Number of requests to process (mandatory)\n");
    printf("  --input:    Input file send to host (mandatory)\n");
    printf("  --host:     Path to the host file (optional)\n");
    printf("  --log:      Path to the log folder (optional)\n");
}


bool cmd_parse_from_args(int argc, char* argv[]) {

    if (argc < 5) {
        printf("Error: --request and --input are mandatory.\n");
        cmd_print_usage(argv[0]);
        return false;
    }

    bool request_found = false;
    bool input_found = false;
    int  ret = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && (i + 1) < argc) {
            ret = config_set_host_file(argv[++i]);
        } else if (strcmp(argv[i], "--log") == 0 && (i + 1) < argc) {
            ret = config_set_log_folder(argv[++i]);
        } else if (strcmp(argv[i], "--request") == 0 && (i + 1) < argc) {
            ret = config_set_request_count(atoi(argv[++i]));
            request_found = true;
        } else if (strcmp(argv[i], "--input") == 0 && (i + 1) < argc) {
            ret = config_set_msg_file(argv[++i]);
            input_found = true;
        } else {
            printf("Error: %s is unknown.\n", argv[i]);
            break;
        }

        if(ret < 0) {
            break;
        }
    }

    if ( (ret >=0 ) && (!request_found || !input_found)) {
        printf("Error: --request and --input are mandatory.\n");
        cmd_print_usage(argv[0]);
        return false;
    }

    return true;
}
