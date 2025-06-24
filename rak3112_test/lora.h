#ifndef LORA_H
#define LORA_H

#include "command.h"

void init_lora_radio();
void handle_at_freq(const AT_Command *cmd);
void handle_at_sf(const AT_Command *cmd);
void handle_at_power(const AT_Command *cmd);
void handle_at_send(const AT_Command *cmd);
void handle_at_cw(const AT_Command *cmd);
void handle_at_cw_stop(const AT_Command *cmd);
void handle_at_preamble(const AT_Command *cmd);
void handle_at_rx_stop(const AT_Command *cmd);
void handle_at_rx(const AT_Command *cmd);
void receive_packet();
#endif // LORA_H