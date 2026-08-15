#include "config.h"
#include <mudmux/mudmux.h>

int main() {
    int exit_code = 0;
    if (mudmux_init("") == 0) {
        mudmux_deinit();
    }
    return exit_code;
}
