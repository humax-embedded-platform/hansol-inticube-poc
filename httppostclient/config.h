#ifndef CONFIG_H
#define CONFIG_H

typedef struct config_item_t
{
    char* msg_file;
    char* host_file;
    char* log_folder;
    int   request_count;
} config_item_t;

void config_init(void);
void config_deinit(void);
// Getters
const char* config_get_msg_file();
const char* config_get_host_file();
const char* config_get_log_folder();
const int   config_get_request_count();

// Setters
int config_set_msg_file(const char* msg_file);
int config_set_host_file(const char* host_file);
int config_set_log_folder(const char* log_folder);
int config_set_request_count(int request_count);

#endif // CONFIG_H
