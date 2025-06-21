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
static uint32_t counter = 0;
static String payload;

float g_lora_freq = CONFIG_RADIO_FREQ;
int g_lora_sf = 10;
int g_lora_power = CONFIG_RADIO_OUTPUT_POWER;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
}



void init_lora_radio() {
    // When the power is turned on, a delay is required.
    delay(1500);

    register_at_handler("AT+FREQ", handle_at_freq, "Set LoRa frequency, e.g. AT+FREQ=868.0");
    register_at_handler("AT+SF", handle_at_sf, "Set LoRa spreading factor, e.g. AT+SF=10");
    register_at_handler("AT+POWER", handle_at_power, "Set LoRa output power, e.g. AT+POWER=22");
    register_at_handler("AT+SEND", handle_at_send, "Send data, e.g. AT+SEND=hello");
    register_at_handler("AT+CW", handle_at_cw, "Start LoRa continuous wave (single carrier)");
    register_at_handler("AT+CWSTOP", handle_at_cw_stop, "Stop LoRa continuous wave (single carrier)");

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
    radio.setPacketSentAction(setFlag);

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

    if (radio.setPreambleLength(8) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
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

void handle_at_freq(const AT_Command *cmd) {
    float freq = atof(cmd->params);
    if (freq >= 137.0 && freq <= 960.0) {
        g_lora_freq = freq;
        radio.setFrequency(g_lora_freq);
        Serial.print("OK, FREQ=");
        Serial.println(g_lora_freq);
    } else {
        Serial.println("ERROR: Invalid FREQ");
    }
}

void handle_at_sf(const AT_Command *cmd) {
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
    if (strlen(cmd->params) == 0) {
        Serial.println("ERROR: No data to send");
        return;
    }
    transmittedFlag = false; // 清除标志
    int state = radio.startTransmit(cmd->params);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("OK, sending...");
        // 等待发送完成
        unsigned long start = millis();
        while (!transmittedFlag && millis() - start < 5000) { // 最多等5秒
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

void handle_at_cw(const AT_Command *cmd) {
    int state = radio.transmitDirect();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("CW mode started.");
    } else {
        Serial.print("ERROR, code ");
        Serial.println(state);
    }
}

void handle_at_cw_stop(const AT_Command *cmd) {
    radio.standby();
    Serial.println("CW mode stopped.");
}

