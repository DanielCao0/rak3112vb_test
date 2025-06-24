#include "lora.h"
#include "utilities.h"
#include "Arduino.h"
#include <RadioLib.h>
#include "command.h"

#define USING_DIO2_AS_RF_SWITCH
#define USING_SX1262


#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           868.0
#endif

#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   22
#endif

#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif

SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// save transmission state between loops
static int transmissionState = RADIOLIB_ERR_NONE;
// flag to indicate that a packet was sent
static volatile bool transmittedFlag = false;
static volatile bool receivedFlag = false;
static uint32_t counter = 0;
static String payload;

static String rssi = "0dBm";
static String snr = "0dB";


float g_lora_freq = CONFIG_RADIO_FREQ;
int g_lora_sf = 10;
int g_lora_power = CONFIG_RADIO_OUTPUT_POWER;
int g_lora_preamble = 8; // add global variable

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setTXFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
}

void setRXFlag(void)
{
    receivedFlag = true;
}



void init_lora_radio() {
    // When the power is turned on, a delay is required.
    delay(1500);

    register_at_handler("AT+PFREQ", handle_at_freq, "Set LoRa frequency, e.g. AT+FREQ=868.0");
    register_at_handler("AT+PSF", handle_at_sf, "Set LoRa spreading factor, e.g. AT+SF=10");
    register_at_handler("AT+PTP", handle_at_power, "Set LoRa output power, e.g. AT+POWER=22");
    register_at_handler("AT+PSEND", handle_at_send, "Send data, e.g. AT+SEND=hello");
    register_at_handler("AT+CW", handle_at_cw, "Start LoRa continuous wave (single carrier)");
    register_at_handler("AT+CWSTOP", handle_at_cw_stop, "Stop LoRa continuous wave (single carrier)");
    register_at_handler("AT+PPL", handle_at_preamble, "Set LoRa preamble length, e.g. AT+PREAMBLE=8");
    register_at_handler("AT+PRECV", handle_at_rx, "Start LoRa receive mode, e.g. AT+RX");
    register_at_handler("AT+RXSTOP", handle_at_rx_stop, "Stop LoRa receive mode, e.g. AT+RXSTOP");

    // initialize radio with default settings
    int state = radio.begin();
  
    Serial.print(F("Radio Initializing ... "));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setPacketSentAction(setTXFlag);
    radio.setPacketReceivedAction(setRXFlag);

    if (radio.setFrequency(g_lora_freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }

    if (radio.setSpreadingFactor(g_lora_sf) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true);
    }

    if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true);
    }

    if (radio.setSyncWord(0x34) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true);
    }

    if (radio.setOutputPower(g_lora_power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }

#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true);
    }
#endif

    if (radio.setPreambleLength(g_lora_preamble) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true);
    }

    if (radio.setCRC(true) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true);
    }

#ifdef USING_DIO2_AS_RF_SWITCH
#ifdef USING_SX1262
    if (radio.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
        Serial.println(F("Failed to set DIO2 as RF switch!"));
        while (true);
    }
    radio.setTCXO(1.8);
#endif //USING_SX1262
#endif //USING_DIO2_AS_RF_SWITCH

    // start transmitting the first packet
    // Serial.print(F("Radio Sending first packet ... "));
    // transmissionState = radio.startTransmit(String(counter).c_str());
    // delay(1000);
}

// Add LoRa busy state
volatile enum LoraState {
    LORA_IDLE = 0,
    LORA_CW,
    LORA_RX
} lora_state = LORA_IDLE;

void handle_at_freq(const AT_Command *cmd) {
    if (strcmp(cmd->params, "?") == 0) {
        Serial.print("Current FREQ: ");
        Serial.println(g_lora_freq, 3);
        return;
    }
    float freq = atof(cmd->params);
    if (freq >= 137.0 && freq <= 960.0) {
        g_lora_freq = freq;
        radio.setFrequency(g_lora_freq);
        Serial.print("OK, FREQ=");
        Serial.println(g_lora_freq, 3);
    } else {
        Serial.println("ERROR: Invalid FREQ");
    }
}

void handle_at_sf(const AT_Command *cmd) {
    if (strcmp(cmd->params, "?") == 0) {
        Serial.print("Current SF: ");
        Serial.println(g_lora_sf);
        return;
    }
    int sf = atoi(cmd->params);
    if (sf >= 5 && sf <= 12) {
        g_lora_sf = sf;
        radio.setSpreadingFactor(g_lora_sf);
        Serial.print("OK, SF=");
        Serial.println(g_lora_sf);
    } else {
        Serial.println("ERROR: Invalid SF");
    }
}

void handle_at_power(const AT_Command *cmd) {
    if (strcmp(cmd->params, "?") == 0) {
        Serial.print("Current POWER: ");
        Serial.println(g_lora_power);
        return;
    }
    int power = atoi(cmd->params);
    if (power >= -9 && power <= 22) {
        g_lora_power = power;
        radio.setOutputPower(g_lora_power);
        Serial.print("OK, POWER=");
        Serial.println(g_lora_power);
    } else {
        Serial.println("ERROR: Invalid POWER");
    }
}

void handle_at_send(const AT_Command *cmd) {
    if (lora_state == LORA_CW) {
        Serial.println("ERROR: Device busy (CW mode)");
        return;
    }
    if (lora_state == LORA_RX) {
        Serial.println("ERROR: Device busy (RX mode)");
        return;
    }
    if (strlen(cmd->params) == 0) {
        Serial.println("ERROR: No data to send");
        return;
    }
    // Check if input is hex string (only 0-9, a-f, A-F, even length)
    const char* p = cmd->params;
    int len = strlen(p);
    bool isHex = (len % 2 == 0);
    for (int i = 0; i < len && isHex; ++i) {
        if (!isxdigit(p[i])) isHex = false;
    }
    if (isHex) {
        // Convert hex string to byte array
        int byteLen = len / 2;
        uint8_t buf[128];
        if (byteLen > 128) {
            Serial.println("ERROR: Data too long");
            return;
        }
        for (int i = 0; i < byteLen; ++i) {
            char tmp[3] = {p[2*i], p[2*i+1], 0};
            buf[i] = (uint8_t)strtol(tmp, NULL, 16);
        }
        transmittedFlag = false;
        int state = radio.startTransmit(buf, byteLen);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("OK, sending HEX...");
            unsigned long start = millis();
            while (!transmittedFlag && millis() - start < 5000) {
                delay(1);
            }
            if (transmittedFlag) {
                Serial.println("SEND DONE");
            } else {
                Serial.println("ERROR: Timeout waiting for TX done");
            }
        } else {
            Serial.print("ERROR, code ");
            Serial.println(state);
        }
    } else {
        // Fallback: send as string
        transmittedFlag = false;
        int state = radio.startTransmit(cmd->params);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("OK, sending...");
            unsigned long start = millis();
            while (!transmittedFlag && millis() - start < 5000) {
                delay(1);
            }
            if (transmittedFlag) {
                Serial.println("SEND DONE");
            } else {
                Serial.println("ERROR: Timeout waiting for TX done");
            }
        } else {
            Serial.print("ERROR, code ");
            Serial.println(state);
        }
    }
}

void handle_at_cw(const AT_Command *cmd) {
    int state = radio.transmitDirect();
    if (state == RADIOLIB_ERR_NONE) {
        lora_state = LORA_CW;
        Serial.println("CW mode started.");
    } else {
        Serial.print("ERROR, code ");
        Serial.println(state);
    }
}

void handle_at_cw_stop(const AT_Command *cmd) {
    radio.standby();
    lora_state = LORA_IDLE;
    Serial.println("CW mode stopped.");
}

void handle_at_preamble(const AT_Command *cmd) {
    if (strcmp(cmd->params, "?") == 0) {
        Serial.print("Current PREAMBLE: ");
        Serial.println(g_lora_preamble);
        return;
    }
    int preamble = atoi(cmd->params);
    if (preamble >= 6 && preamble <= 65535) {
        g_lora_preamble = preamble;
        if (radio.setPreambleLength(g_lora_preamble) == RADIOLIB_ERR_NONE) {
            Serial.print("OK, PREAMBLE=");
            Serial.println(g_lora_preamble);
        } else {
            Serial.println("ERROR: Failed to set preamble length");
        }
    } else {
        Serial.println("ERROR: Invalid PREAMBLE");
    }
}


void receive_packet() {
    if (receivedFlag) {

        // reset flag
        receivedFlag = false;

        // read received data as byte array
        uint8_t byteArr[256];
        int len = radio.getPacketLength();
        int state = radio.readData(byteArr, len);

        if (state == RADIOLIB_ERR_NONE) {
            rssi = String(radio.getRSSI()) + "dBm";
            snr = String(radio.getSNR()) + "dB";

            Serial.println(F("Radio Received packet!"));
            Serial.print(F("Radio Data (HEX):"));
            for (int i = 0; i < len; i++) {
                if (byteArr[i] < 16) Serial.print("0");
                Serial.print(byteArr[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            Serial.print(F("Radio RSSI:"));
            Serial.println(rssi);
            Serial.print(F("Radio SNR:"));
            Serial.println(snr);
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            Serial.println(F("CRC error!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        radio.startReceive();

    }
}

void handle_at_rx(const AT_Command *cmd) {
    Serial.print(F("Radio Starting to listen ... "));
    int state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        lora_state = LORA_RX;
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }
}

void handle_at_rx_stop(const AT_Command *cmd) {
    radio.standby();
    lora_state = LORA_IDLE;
    Serial.println("LoRa RX mode stopped.");
}
