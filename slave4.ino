#include <esp_now.h>
#include <WiFi.h>

// Struktur data untuk menerima perintah dari ESP utama
typedef struct struct_message {
  int command;            // Perintah dari ESP utama (1: Hidupkan, 0: Matikan)
} struct_message_in;

// Struktur data untuk dikirim ke ESP utama
typedef struct struct_response {
  int lampuID;            // ID lampu/slave (1 - 4)
  unsigned long waktu;    // Waktu reaksi dalam milidetik
} struct_message_out;

// Variabel data
struct_message_in receivedData;
struct_message_out sendData;

// ID unik untuk setiap ESP slave
int lampuID = 4; // Ganti 1, 2, 3, atau 4 sesuai slave

// Pin untuk sensor dan LED
#define SENSOR 33          // Sensor proximity
#define LED 4              // LED kecil

// Alamat MAC ESP utama
uint8_t broadcastAddress[] = {0x8c, 0x4f, 0x00, 0x10, 0xda, 0xcc};

// Variabel untuk menyimpan waktu reaksi
unsigned long startTime = 0;

// Callback ketika data diterima
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  if (receivedData.command == 1) {
    // Hidupkan LED dan catat waktu mulai
    digitalWrite(LED, HIGH);
    startTime = millis();

    // Tunggu sampai sensor proximity mendeteksi tangan (jarak < 5 cm)
    while (digitalRead(SENSOR) == HIGH) {
      // Tunggu
    }

    unsigned long reactionTime = millis() - startTime; // Hitung waktu reaksi
    digitalWrite(LED, LOW);  // Matikan LED

    // Siapkan data untuk dikirim ke ESP utama
    sendData.lampuID = lampuID;
    sendData.waktu = reactionTime;

    // Kirim data ke ESP utama
    esp_now_send(broadcastAddress, (uint8_t *)&sendData, sizeof(sendData));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Loop kosong
}
