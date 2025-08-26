#ifndef BLE_H
#define BLE_H

#include "command.h"

void handle_at_blescan(const AT_Command *cmd);
void init_ble();


#endif // BLE_H