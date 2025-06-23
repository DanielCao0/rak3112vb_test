#ifndef BLE_H
#define BLE_H

#include "command.h"

void handle_at_blescan(const AT_Command *cmd);
void ble_init();


#endif // BLE_H