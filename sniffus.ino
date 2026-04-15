
#include <ESP8266WiFi.h>


struct RxControl {
 signed rssi: 8;
 unsigned rate: 4;
 unsigned is_group: 1;
 unsigned: 1;
 unsigned sig_mode: 2;
 unsigned legacy_length: 12;
 unsigned damatch0: 1;
 unsigned damatch1: 1;
 unsigned bssmatch0: 1;
 unsigned bssmatch1: 1;
 unsigned MCS: 7;
 unsigned CWB: 1;
 unsigned HT_length: 16;
 unsigned Smoothing: 1;
 unsigned Not_Sounding: 1;
 unsigned: 1;
 unsigned Aggregation: 1;
 unsigned STBC: 2;
 unsigned FEC_CODING: 1;
 unsigned SGI: 1;
 unsigned rx_fill: 8;
};

struct SnifferPacket {
 struct RxControl rx_ctrl;
 uint8_t data[128]; 
};

void sniffer(uint8_t *buf, uint16_t len) {
  struct SnifferPacket *pkt = (struct SnifferPacket *)buf;
  uint8_t *payload = pkt->data;
  int rssi = pkt->rx_ctrl.rssi;

  // 1. PROBE REQUEST (0x40)
  if (payload[0] == 0x40) {
    String client = "";
    for (int i = 10; i < 16; i++) client += String(payload[i] < 0x10 ? "0" : "") + String(payload[i], HEX);
    
    int ssidLen = payload[25];
    String ssid = "";
    if (ssidLen > 0 && ssidLen < 33) {
      for (int i = 0; i < ssidLen; i++) {
        char c = (char)payload[26 + i];
        if (c >= 32 && c <= 126) ssid += c;
      }
      // Format P:MAC|SSID|RSSI
      Serial.println("P:" + client + "|" + ssid + "|" + String(rssi));
    }
  }
  

  String bssid = ""; String client = "";
  for (int i = 16; i < 22; i++) bssid += String(payload[i] < 0x10 ? "0" : "") + String(payload[i], HEX);
  for (int i = 10; i < 16; i++) client += String(payload[i] < 0x10 ? "0" : "") + String(payload[i], HEX);
  
  if (bssid != "ffffffffffff" && client != bssid && client.length() == 12) {
    // Format L:BSSID|CLIENT|RSSI
    Serial.println("L:" + bssid + "|" + client + "|" + String(rssi));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer);
  wifi_promiscuous_enable(1);
  
  Serial.println("\n[SYSTEM]  ACTIVE");
}

void loop() {
  static unsigned long lastChan = 0;
  static int ch = 1;
  
 
  if (millis() - lastChan > 150) {
    ch++; 
    if (ch > 13) ch = 1;
    wifi_set_channel(ch);
    lastChan = millis();
  }


  static unsigned long lastBeaconScan = 0;
  if (millis() - lastBeaconScan > 8000) {
    wifi_promiscuous_enable(0);
    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++) {
      // Format R:SSID|BSSID|CH|RSSI
      Serial.println("R:" + WiFi.SSID(i) + "|" + WiFi.BSSIDstr(i) + "|" + String(WiFi.channel(i)) + "|" + String(WiFi.RSSI(i)));
    }
    wifi_promiscuous_enable(1);
    lastBeaconScan = millis();
  }
}