/*
 * pin 1 - not used          |  Micro SD card     |
 * pin 2 - CS (SS)           |                   /
 * pin 3 - DI (MOSI)         |                  |__
 * pin 4 - VDD (3.3V)        |                    |
 * pin 5 - SCK (SCLK)        | 8 7 6 5 4 3 2 1   /
 * pin 6 - VSS (GND)         | ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄  /
 * pin 7 - DO (MISO)         | ▀ ▀ █ ▀ █ ▀ ▀ ▀ |
 * pin 8 - not used          |_________________|
 *                             ║ ║ ║ ║ ║ ║ ║ ║
 *                     ╔═══════╝ ║ ║ ║ ║ ║ ║ ╚═════════╗
 *                     ║         ║ ║ ║ ║ ║ ╚══════╗    ║
 *                     ║   ╔═════╝ ║ ║ ║ ╚═════╗  ║    ║
 * Connections for     ║   ║   ╔═══╩═║═║═══╗   ║  ║    ║
 * full-sized          ║   ║   ║   ╔═╝ ║   ║   ║  ║    ║
 * SD card             ║   ║   ║   ║   ║   ║   ║  ║    ║
 * Pin name         |  -  DO  VSS SCK VDD VSS DI CS    -  |
 * SD pin number    |  8   7   6   5   4   3   2   1   9 /
 *                  |                                  █/
 *                  |__▍___▊___█___█___█___█___█___█___/
 *
 *
 *
 *
 * Note:  The SPI pins can be manually configured by using `SPI.begin(sck, miso, mosi, cs).`
 *        Alternatively, you can change the CS pin and use the other default settings by using `SD.begin(cs)`.
 *
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SPI Pin Name | ESP8266 | ESP32 | ESP32‑S2 | ESP32‑S3 | ESP32‑C3 | ESP32‑C6 | ESP32‑H2 |
 * +==============+=========+=======+==========+==========+==========+==========+==========+
 * | CS (SS)      | GPIO15  | GPIO5 | GPIO34   | GPIO10   | GPIO7    | GPIO18   | GPIO0    |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DI (MOSI)    | GPIO13  | GPIO23| GPIO35   | GPIO11   | GPIO6    | GPIO19   | GPIO25   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DO (MISO)    | GPIO12  | GPIO19| GPIO37   | GPIO13   | GPIO5    | GPIO20   | GPIO11   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SCK (SCLK)   | GPIO14  | GPIO18| GPIO36   | GPIO12   | GPIO4    | GPIO21   | GPIO10   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 *
 * For more info see file README.md in this library or on URL:
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
 */

// SD Library
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// VESC - UART Library
#include <VescUart.h>
#include <buffer.h>
#include <crc.h>
#include <datatypes.h>

#include "ESPTelnet.h"


#define STOP_DATABASE_PIN 3
#define COMETS_FILE_VERSION 01

/* ------------------------------------------------- */

#define SERIAL_SPEED 115200
#define WIFI_SSID "COMETS"
#define WIFI_PASSWORD "MotoElec"

/* ------------------------------------------------- */
//Comment to stop telnet connection
#define TELNET_USE

#ifdef TELNET_USE
ESPTelnet telnet;
IPAddress ip;
uint16_t port = 23;
bool is_telnet_user_connected = false;
#endif

/* ------------------------------------------------- */


// Uncomment and set up if you want to use custom pins for the SPI communication
#define REASSIGN_PINS
int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;

#define RX_PIN 13  // Replace with your actual RX pin
#define TX_PIN 4   // Replace with your actual TX pin

//TODO more error to add
enum ERROR {
  VESCUART_CONNECTION_ISSUE,
  ERROR_TODO,
  ERROR_TODO2
};

std::vector<String> fileNames;  // Vector to store filenames
File logFile;
String logFileName;  // Variable for the File name

VescUart UART;  // UART constant to access VESC - UART library



String getNextFileName(fs::FS &fs, const char *baseName, const char *extension) {
  uint16_t fileIndex = 0;  // Start with 0
  String fileName;

  while (true) {
    if (fileIndex == 0) {
      fileName = String(baseName) + "." + extension;  // For the first file: "data.csv"
    } else {
      fileName = String(baseName) + String(fileIndex) + "." + extension;  // For example: "data1.csv"
    }

    // Check if the file exists
    if (!fs.exists(fileName)) {
      break;  // Found an available name
    }
    fileIndex++;  // Increment index and check next name
  }

  return fileName;  // Return the available filename
}

unsigned long StartTime = 0;
unsigned long CurrentTime = 0;
void setup() {
  // put your setup code here, to run once:
  /** Setup Serial port to display data */
  Serial.begin(115200);

  /** Setup UART port (Serial1 on Atmega32u4) */
  // HardwareSerial Serial1(1);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  while (!Serial) { ; }

  Serial.print("- Wifi: ");
#ifdef TELNET_USE
  telnet_init();
#endif

  /** Define which ports to use as UART */
  UART.setSerialPort(&Serial1);

#ifdef REASSIGN_PINS
  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
#else
  if (!SD.begin()) {
#endif
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  StartTime = millis();  // start saving time

  //Setup the stop pin 
  pinMode(STOP_DATABASE_PIN, INPUT_PULLUP);
}

void loop() {
// put your main code here, to run repeatedly:
#ifdef TELNET_USE
  telnet.loop();
#endif

  if (UART.getVescValues()) {
    // Write data to log file
    CurrentTime = millis();
    save_data('t', 1, UART.data.tempMotor, CurrentTime - StartTime);
    save_data('r', 1, UART.data.rpm, CurrentTime - StartTime);
    save_data('v', 1, UART.data.inpVoltage, CurrentTime - StartTime);
    save_data('a', 1, UART.data.ampHours, CurrentTime - StartTime);
    // save_data('?', 1, UART.data.avgMotorCurrent, CurrentTime - StartTime); //TODO
  } else {
    CurrentTime = millis();
    Serial.println("UART Connection Error with VESC");
    save_data('e', 1, VESCUART_CONNECTION_ISSUE, CurrentTime - StartTime);
    return;  // Skip further processing
  }
  delay(1000);  //TODO REMOVE DELAY for other function
  if(!digitalRead(STOP_DATABASE_PIN))
  {
    Serial.println("Stopping all communication & Saving database");
    logFile.close();
    telnet.println("#END_OF_LOG");
    telnet.stop();
    //infinit loop TODO: user need to be able to retart
    while(1){}
  }
}

// void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
//   Serial.printf("Listing directory: %s\n", dirname);

//   File root = fs.open(dirname);
//   if (!root) {
//     Serial.println("Failed to open directory");
//     return;
//   }
//   if (!root.isDirectory()) {
//     Serial.println("Not a directory");
//     return;
//   }

//   File file = root.openNextFile();
//   while (file) {
//     if (file.isDirectory()) {
//       Serial.print("  DIR : ");
//       Serial.println(file.name());
//       if (levels) {
//         listDir(fs, file.path(), levels - 1);
//       }
//     } else {
//       Serial.print("  FILE: ");
//       Serial.print(file.name());
//       Serial.print("  SIZE: ");
//       Serial.println(file.size());
//       // Store filename in the vector
//       fileNames.push_back(String(file.name()));
//     }
//     file = root.openNextFile();
//   }
// }

// void createDir(fs::FS &fs, const char *path) {
//   Serial.printf("Creating Dir: %s\n", path);
//   if (fs.mkdir(path)) {
//     Serial.println("Dir created");
//   } else {
//     Serial.println("mkdir failed");
//   }
// }

// void removeDir(fs::FS &fs, const char *path) {
//   Serial.printf("Removing Dir: %s\n", path);
//   if (fs.rmdir(path)) {
//     Serial.println("Dir removed");
//   } else {
//     Serial.println("rmdir failed");
//   }
// }

// void readFile(fs::FS &fs, const char *path) {
//   Serial.printf("Reading file: %s\n", path);

//   File file = fs.open(path);
//   if (!file) {
//     Serial.println("Failed to open file for reading");
//     return;
//   }

//   Serial.print("Read from file: ");
//   while (file.available()) {
//     Serial.write(file.read());
//   }
//   file.close();
// }

// void writeFile(fs::FS &fs, const char *path, const char *message) {
//   Serial.printf("Writing file: %s\n", path);

//   File file = fs.open(path, FILE_WRITE);
//   if (!file) {
//     Serial.println("Failed to open file for writing");
//     return;
//   }
//   if (file.print(message)) {
//     Serial.println("File written");
//   } else {
//     Serial.println("Write failed");
//   }
//   file.close();
// }

// void appendFile(fs::FS &fs, const char *path, const char *message) {
//   Serial.printf("Appending to file: %s\n", path);

//   File file = fs.open(path, FILE_APPEND);
//   if (!file) {
//     Serial.println("Failed to open file for appending");
//     return;
//   }
//   if (file.print(message)) {
//     Serial.println("Message appended");
//   } else {
//     Serial.println("Append failed");
//   }
//   file.close();
// }

// void renameFile(fs::FS &fs, const char *path1, const char *path2) {
//   Serial.printf("Renaming file %s to %s\n", path1, path2);
//   if (fs.rename(path1, path2)) {
//     Serial.println("File renamed");
//   } else {
//     Serial.println("Rename failed");
//   }
// }

// void deleteFile(fs::FS &fs, const char *path) {
//   Serial.printf("Deleting file: %s\n", path);
//   if (fs.remove(path)) {
//     Serial.println("File deleted");
//   } else {
//     Serial.println("Delete failed");
//   }
// }

// bool checkFile(const char *fileName) {
//   bool found = false;
//   for (const auto &name : fileNames) {
//     if (name.equals(fileName)) {
//       Serial.printf("File %s found!\n", fileName);
//       found = true;
//       break;
//     }
//   }
//   if (!found) {
//     Serial.printf("File %s not found.\n", fileName);
//   }

//   return found;
// }