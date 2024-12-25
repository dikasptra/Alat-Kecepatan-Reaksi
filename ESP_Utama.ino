#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

// Konfigurasi LCD I2C
LiquidCrystal_PCF8574 lcd(0x27); // Alamat I2C LCD

// Struktur data untuk mengirim data ke slave
typedef struct struct_message {
  int command; // Perintah untuk slave (1: Hidupkan, 0: Matikan)
} struct_message;

// Struktur data untuk menerima data dari slave
typedef struct struct_response {
  int lampuID;          // ID lampu/slave (1 - 4)
  unsigned long waktu;  // Waktu reaksi dalam milidetik
} struct_response;

// Variabel untuk data
struct_message sendData;
struct_response receivedData;

// Variabel untuk tombol
const int buttonPins[] = {12, 13, 14, 27, 33}; // GPIO untuk tombol
const int numButtons = 5;
bool buttonPressed[numButtons] = {false, false, false, false, false};

// Data waktu dari setiap slave
unsigned long reactionTimes[4] = {0, 0, 0, 0};

// Alamat MAC ESP slave
uint8_t slaveMAC[4][6] = {
    {0x7c, 0x9e, 0xbd, 0x06, 0xf0, 0x5c}, // MAC Slave 1
    {0x7c, 0x9e, 0xbd, 0x06, 0x4f, 0x30}, // MAC Slave 2
    {0xd0, 0xef, 0x76, 0x32, 0x3d, 0x04}, // MAC Slave 3
    {0xac, 0x15, 0x18, 0xd8, 0x42, 0x38}  // MAC Slave 4
};

// Callback ketika data diterima
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  // Simpan data waktu reaksi
  if (receivedData.lampuID >= 1 && receivedData.lampuID <= 4) {
    reactionTimes[receivedData.lampuID - 1] = receivedData.waktu;

    // Perbarui tampilan LCD
    lcd.clear();
    for (int i = 0; i < 4; i++) {
      lcd.setCursor(i % 2 == 0 ? 0 : 8, i < 2 ? 0 : 1);
      lcd.print("L");
      lcd.print(i + 1);
      lcd.print(":");
      lcd.print(reactionTimes[i]);
    }
  }
}

// Callback ketika data dikirim
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Data sent to MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Fungsi untuk mengirim data ke slave
void sendDataToSlave(int slaveID, int command) {
  sendData.command = command;
  esp_now_send(slaveMAC[slaveID], (uint8_t *)&sendData, sizeof(sendData));
}

void resetData() {
  // Reset data waktu
  for (int i = 0; i < 4; i++) {
    reactionTimes[i] = 0;
  }

  // Reset status tombol
  for (int i = 0; i < numButtons; i++) {
    buttonPressed[i] = false;
  }

  // Bersihkan LCD
  lcd.clear();
  lcd.print("Menunggu data...");
}

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(115200);

  // Inisialisasi LCD
  lcd.begin(16, 2, Wire);
  lcd.setBacklight(255);
  lcd.clear();
  lcd.print("Menunggu data...");
  
  
  // Atur tombol sebagai input pull-up
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Atur mode Wi-Fi ke Station
  WiFi.mode(WIFI_STA);

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    lcd.clear();
    lcd.print("ESP-NOW Error!");
    return;
  }

  // Daftarkan callback untuk pengiriman dan penerimaan data
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Tambahkan peer (slave)
  for (int i = 0; i < 4; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, slaveMAC[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
    }
  }
}

void tampilkanHasil() {
  lcd.clear();
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(i % 2 == 0 ? 0 : 8, i < 2 ? 0 : 1);
    lcd.print("L");
    lcd.print(i + 1);
    lcd.print(":");
    lcd.print(reactionTimes[i]);
    lcd.print("   "); // Tambahkan spasi untuk menghapus sisa karakter sebelumnya
  }
  delay(100); // Tambahkan delay 1 detik
}

void loop() {
  // Baca status tombol
  for (int i = 0; i < numButtons; i++) {
    if (digitalRead(buttonPins[i]) == LOW && !buttonPressed[i]) {
      delay(200); // Debounce tombol
      buttonPressed[i] = true;

      if (i < 4) {
        // Kirim perintah ke slave untuk menghidupkan lampu
        sendDataToSlave(i, 1);
      } else if (i == 4) {
        // Tombol reset
        resetData();
      }
    }
  }

  // Periksa apakah semua tombol telah ditekan
  bool allPressed = true;
  for (int i = 0; i < 4; i++) {
    if (!buttonPressed[i]) {
      allPressed = false;
      break;
    }
  }

  // Jika semua tombol telah ditekan, tampilkan data akhir
  if (allPressed) {
    tampilkanHasil();
  }
}
