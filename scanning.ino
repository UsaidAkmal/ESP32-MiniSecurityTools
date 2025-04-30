/*saya menajalankan kode ini menggunakan Arduino IDE dengan menggunakan bahasa pemrograman c++*/

/*REQUIREMENT*/
/**/

#include "WiFi.h"

bool scanningActive = true;
String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("Setup selesai. Scanning aktif.");
  Serial.println("Ketik 'stop' untuk menghentikan atau 'start' untuk memulai kembali");
}

void loop() {
  // Proses perintah serial
  if (stringComplete) {
    inputString.trim();
    
    if (inputString.equalsIgnoreCase("stop")) {
      scanningActive = false;
      Serial.println("Scanning dihentikan");
    } 
    else if (inputString.equalsIgnoreCase("start")) {
      scanningActive = true;
      Serial.println("Scanning dimulai kembali");
    }
    
    // Reset string untuk perintah berikutnya
    inputString = "";
    stringComplete = false;
  }
  
  // Lakukan pemindaian jika aktif
  if (scanningActive) {
    scanNetworks();
  } else {
    // Periksa input Serial saat tidak aktif
    checkSerialInput();
    delay(100);
  }
}

void scanNetworks() {
  Serial.println("Memulai pemindaian jaringan WiFi...");
  
  // Lakukan pemindaian
  int networkCount = WiFi.scanNetworks();
  
  if (networkCount == 0) {
    Serial.println("Tidak ada jaringan WiFi yang ditemukan");
  } else {
    Serial.print(networkCount);
    Serial.println(" jaringan WiFi ditemukan");
    
    // Tampilkan detail jaringan
    for (int i = 0; i < networkCount; ++i) {
      // Format: SSID, RSSI, Encryption Type, MAC Address
      String encType = "";
      switch(WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN: encType = "Open"; break;
        case WIFI_AUTH_WEP: encType = "WEP"; break;
        case WIFI_AUTH_WPA_PSK: encType = "WPA-PSK"; break;
        case WIFI_AUTH_WPA2_PSK: encType = "WPA2-PSK"; break;
        case WIFI_AUTH_WPA_WPA2_PSK: encType = "WPA/WPA2-PSK"; break;
        case WIFI_AUTH_WPA2_ENTERPRISE: encType = "WPA2-Enterprise"; break;
        default: encType = "Unknown"; break;
      }
      
      Serial.print("Network ");
      Serial.print(i+1);
      Serial.print(": ");
      Serial.print("SSID: ");
      Serial.print(WiFi.SSID(i));
      Serial.print(", RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm, Security: ");
      Serial.print(encType);
      Serial.print(", BSSID: ");
      Serial.println(WiFi.BSSIDstr(i));
      
      // Periksa input serial untuk menanggapi perintah stop segera
      checkSerialInput();
      if (!scanningActive) break;
      
      delay(10);
    }
  }
  
  Serial.println("");
  
  // Menunggu sebelum pemindaian berikutnya
  for (int i = 0; i < 50; i++) {
    checkSerialInput();
    if (!scanningActive) break;
    delay(100);
  }
}

void checkSerialInput() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void serialEvent() {
  checkSerialInput();
}