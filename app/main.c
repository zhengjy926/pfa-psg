#include "app/psg_app.h"
#include "bsp/psg_v3/psg_board.h"

int main(void)
{
    psg_board_init();
    psg_app_init();
    psg_app_start();
    for (;;) {
    }
}
