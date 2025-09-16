#ifndef LORA_H
#define LORA_H

#include "command.h"

// Radio mode definitions
#define RADIO_MODE_LORA 0
#define RADIO_MODE_FSK  1

// FSK radio configuration structure
struct FSK_Config {
    float freq;
    int power;
    int bitrate;
    int deviation;
};

extern int g_radio_mode; // Global radio mode variable

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

// Shared functions for both LoRa and FSK
void handle_at_bandwidth(const AT_Command *cmd);

// FSK functions
void init_fsk_radio();
void set_fsk_freq(float freq);
int fsk_send_packet(const char* data, int len);
void handle_at_fsk_send(const AT_Command *cmd);
void handle_at_mode(const AT_Command *cmd);
void handle_at_fsk_bitrate(const AT_Command *cmd);
void handle_at_fsk_deviation(const AT_Command *cmd);

#ifdef __cplusplus
extern "C" {
#endif
void fhss_auto_hop_send_loop();
#ifdef __cplusplus
}
#endif

#endif // LORA_H