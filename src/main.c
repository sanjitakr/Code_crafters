#include "ui.h"
#include "network.h"
#include "persistence.h"
#include "auth.h"
#include "utils.h"

int main(int argc, char **argv) {
    utils_log("Starting Collaborative Editor...");

    persistence_init();
    network_init("localhost", 8080);
    ui_init(argc, argv);

    ui_loop();

    network_shutdown();
    return 0;
}
