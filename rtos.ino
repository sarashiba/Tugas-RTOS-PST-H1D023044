#include <Arduino_FreeRTOS.h>
#include <queue.h>

/**
 * Penentuan Pin Komponen
 * Pin A0: Input Analog dari Modul LDR
 * Pin 8 : Output Digital untuk LED
 */
const int ldrAnalogPin = A0; 
const int ledPin = 8;

/**
 * Sinkronisasi Antar-Task
 * Menggunakan QueueHandle_t untuk mengirimkan data status cahaya
 * dari Task Pengamat ke Task Eksekutor secara real-time.
 */
QueueHandle_t lightQueue;

// Deklarasi fungsi prototipe Task
void TaskAmatiCahaya(void *pvParameters);
void TaskKontrolLampu(void *pvParameters);

void setup() {
  // Inisialisasi komunikasi serial untuk monitoring sistem
  Serial.begin(9600);
  
  pinMode(ledPin, OUTPUT);

  // Inisialisasi Queue untuk menampung 1 data berukuran integer
  lightQueue = xQueueCreate(1, sizeof(int));

  // Validasi pembuatan Queue sebelum menjalankan Scheduler
  if (lightQueue != NULL) {
    /**
     * Pembentukan Task RTOS
     * Task 1: Prioritas 1 (Membaca sensor LDR)
     * Task 2: Prioritas 2 (Mengontrol LED - Prioritas lebih tinggi untuk responsivitas)
     */
    xTaskCreate(TaskAmatiCahaya, "BacaLDR", 128, NULL, 1, NULL);
    xTaskCreate(TaskKontrolLampu, "Lampu", 128, NULL, 2, NULL);
  }
}

void loop() {
  // Loop utama dikosongkan karena manajemen tugas dilakukan oleh RTOS Scheduler
}

/**
 * TASK 1: Pengamat Cahaya (Producer)
 * Bertugas membaca nilai analog dari modul LDR secara berkala.
 */
void TaskAmatiCahaya(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    int nilaiCahaya = analogRead(ldrAnalogPin);
    
    /**
     * Logika Threshold:
     * Nilai > 600 diidentifikasi sebagai kondisi gelap.
     */
    int statusGelap = (nilaiCahaya > 600) ? 1 : 0; 

    Serial.print("Monitoring - Nilai LDR: ");
    Serial.println(nilaiCahaya);

    /**
     * Mengirimkan hasil pembacaan ke Task 2 melalui Queue.
     * Penggunaan portMAX_DELAY memastikan sinkronisasi yang ketat.
     */
    xQueueSend(lightQueue, &statusGelap, portMAX_DELAY);
    
    // Memberikan jeda waktu sistem sebesar 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

/**
 * TASK 2: Kontrol Lampu (Consumer)
 * Task ini bekerja secara sinkron dan hanya akan aktif jika menerima data dari Task 1.
 */
void TaskKontrolLampu(void *pvParameters) {
  (void) pvParameters;
  int terimaStatus;

  for (;;) {
    /**
     * xQueueReceive akan membuat task berada dalam kondisi 'Blocked' (diam) 
     * sampai terdapat data masuk ke dalam antrean.
     */
    if (xQueueReceive(lightQueue, &terimaStatus, portMAX_DELAY) == pdPASS) {
      
      if (terimaStatus == 1) {
        digitalWrite(ledPin, HIGH); 
        Serial.println("System Log: Kondisi Gelap - Lampu Menyala");
      } else {
        digitalWrite(ledPin, LOW);  
        Serial.println("System Log: Kondisi Terang - Lampu Mati");
      }
    }
  }
}