#include "lora.h"
#include "utilities.h"
#include "Arduino.h"
#include <RadioLib.h>
#include "command.h"

#include <stdlib.h>
#include <vector>
#include <algorithm>

// 自动跳频发送控制
volatile bool fhss_auto_send = false;
unsigned long fhss_last_hop = 0;
unsigned long fhss_hop_interval_ms = 1000; // 1000ms跳一次
const char* fhss_send_data = "FHSS_TEST";
std::vector<size_t> fhss_channel_order; // 随机顺序
size_t fhss_channel_idx = 0;

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

// 跳频参数和信道表
static float fh_start_freq = 902.3;
static float fh_end_freq = 914.9;
static float fh_step = 0.2;
static int fh_bw = 125;
static int fh_num = 64;
static std::vector<float> fh_channels;

void build_fh_channels() {
    fh_channels.clear();
    float freq = fh_start_freq;
    for (int i = 0; i < fh_num; ++i) {
        fh_channels.push_back(freq);
        freq += fh_step;
        if (freq > fh_end_freq) break;
    }
    // 若信道数不足，补足
    while (fh_channels.size() < (size_t)fh_num && fh_channels.size() > 0) {
        fh_channels.push_back(fh_channels.back());
    }
}

// 生成随机顺序
void build_fhss_channel_order() {
    fhss_channel_order.clear();
    for (size_t i = 0; i < fh_channels.size(); ++i) fhss_channel_order.push_back(i);
    std::random_shuffle(fhss_channel_order.begin(), fhss_channel_order.end());
    fhss_channel_idx = 0;
}

void random_hop_channel() {
    if (fh_channels.empty()) build_fh_channels();
    if (fhss_channel_order.empty()) build_fhss_channel_order();
    if (fhss_channel_idx >= fhss_channel_order.size()) return;
    size_t idx = fhss_channel_order[fhss_channel_idx];
    float freq = fh_channels[idx];
    radio.setFrequency(freq);
    Serial.print("Hopped to channel: ");
    Serial.println(((int)(freq * 10 + 0.5)) / 10.0, 1);
}

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
    Serial.println("TX Done");
    
}


void setRXFlag(void)
{
    receivedFlag = true;
}



void init_lora_radio() {
    // When the power is turned on, a delay is required.
    delay(1500);

    register_at_handler("AT+PFREQ", handle_at_freq, "Set/query LoRa frequency, e.g. AT+PFREQ=868.0 or AT+PFREQ=?");
    register_at_handler("AT+PSF", handle_at_sf, "Set/query LoRa spreading factor, e.g. AT+PSF=10 or AT+PSF=?");
    register_at_handler("AT+PTP", handle_at_power, "Set/query LoRa output power, e.g. AT+PTP=22 or AT+PTP=?");
    register_at_handler("AT+PSEND", handle_at_send, "Send data, e.g. AT+PSEND=hello or AT+PSEND=112233");
    register_at_handler("AT+CW", handle_at_cw, "Start LoRa continuous wave (single carrier)");
    register_at_handler("AT+CWSTOP", handle_at_cw_stop, "Stop LoRa continuous wave (single carrier)");
    register_at_handler("AT+PPL", handle_at_preamble, "Set/query LoRa preamble length, e.g. AT+PPL=8 or AT+PPL=?");
    register_at_handler("AT+PPREAMBLE", handle_at_preamble, "Set/query LoRa preamble length, e.g. AT+PPREAMBLE=8 or AT+PPREAMBLE=?");
    register_at_handler("AT+PRECV", handle_at_rx, "Start LoRa receive mode");
    register_at_handler("AT+RXSTOP", handle_at_rx_stop, "Stop LoRa receive mode");
    register_at_handler("AT+FHSET", handle_at_fhset, "Set/query FHSS params: AT+FHSET=start,end,step,bw,num e.g. AT+FHSET=902.3,914.9,0.2,125,64 or AT+FHSET=?");

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
    
    //radio.setPacketSentAction(setFlag);   //果然后面会覆盖前面的   其实这两个函数注册的是一个接口
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
            Serial.println("OK");
            // unsigned long start = millis();
            // while (!transmittedFlag && millis() - start < 5000) {
            //     delay(1);
            // }
            // if (transmittedFlag) {
            //     Serial.println("SEND DONE");
            // } else {
            //     Serial.println("ERROR: Timeout waiting for TX done");
            // }
        } else {
            Serial.print("ERROR, code ");
            Serial.println(state);
        }
    } else {
        // Fallback: send as string
        transmittedFlag = false;
        //radio.setPacketSentAction(setFlag);
        int state = radio.startTransmit(cmd->params);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("OK, sending...");
            // unsigned long start = millis();
            // while (!transmittedFlag && millis() - start < 5000) {
            //     delay(1);
            // }
            // if (transmittedFlag) {
            //     Serial.println("SEND DONE");
            // } else {
            //     Serial.println("ERROR: Timeout waiting for TX done");
            // }
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
    if (receivedFlag && lora_state == LORA_RX) {
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
    //radio.setPacketReceivedAction(setRXFlag);
    receivedFlag = false;
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

// AT+FHSET=902.3,914.9,0.2,125,64  或 AT+FHSET=?
void handle_at_fhset(const AT_Command *cmd) {
    if (strcmp(cmd->params, "?") == 0) {
        Serial.print("FHSS: start="); Serial.print(fh_start_freq, 3);
        Serial.print(", end="); Serial.print(fh_end_freq, 3);
        Serial.print(", step="); Serial.print(fh_step, 3);
        Serial.print(", bw="); Serial.print(fh_bw);
        Serial.print(", num="); Serial.println(fh_num);
        Serial.print("Channels: ");
        for (size_t i = 0; i < fh_channels.size(); ++i) {
            Serial.print(fh_channels[i], 3); Serial.print(" ");
        }
        Serial.println();
        return;
    }
    // 解析参数
    char buf[128];
    strncpy(buf, cmd->params, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    char *p = strtok(buf, ",");
    float vals[5] = {0};
    int idx = 0;
    while (p && idx < 5) {
        vals[idx++] = atof(p);
        p = strtok(NULL, ",");
    }
    if (idx < 5) {
        Serial.println("ERROR: Need 5 params: start,end,step,bw,num");
        return;
    }
    fh_start_freq = vals[0];
    fh_end_freq = vals[1];
    fh_step = vals[2];
    fh_bw = (int)vals[3];
    fh_num = (int)vals[4];
    if (fh_step <= 0 || fh_bw <= 0 || fh_num <= 0) {
        Serial.println("ERROR: Invalid FHSS params");
        return;
    }
    build_fh_channels();
    build_fhss_channel_order();
    Serial.print("OK, FHSS set. Channels: ");
    for (size_t i = 0; i < fh_channels.size(); ++i) {
        Serial.print(fh_channels[i], 3); Serial.print(" ");
    }
    Serial.println();

    // 设置完成后自动跳频发送
    fhss_auto_send = true;
    fhss_last_hop = millis();
    Serial.println("FHSS auto hopping and sending started.");
}

// 自动跳频发送函数（主循环中调用）
void fhss_auto_hop_send_loop() {
    if (!fhss_auto_send) return;
    unsigned long now = millis();
    if (now - fhss_last_hop >= fhss_hop_interval_ms) {
        if (fhss_channel_idx >= fhss_channel_order.size()) {
            fhss_auto_send = false;
            Serial.println("FHSS all channels sent, auto hopping stopped.");
            return;
        }
        random_hop_channel();
        // 发送测试数据
        int state = radio.startTransmit(fhss_send_data);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.print("FHSS TX OK, channel idx: ");
            Serial.println(fhss_channel_idx);
        } else {
            Serial.print("FHSS TX ERROR, code ");
            Serial.println(state);
        }
        fhss_channel_idx++;
        fhss_last_hop = now;
    }
}
