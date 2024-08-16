#include "logrecv.h"

int main() {
    logrecv_t receiver;

    logrecv_init(&receiver);
    logrecv_deinit(&receiver);
}