#include "logrecv.h"
#include "logconfig.h"

int main() {
    logrecv_t receiver;

    logconfig_init();
    logrecv_init(&receiver);
    logrecv_deinit(&receiver);
    logconfig_deinit();
}