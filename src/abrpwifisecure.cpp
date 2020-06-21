// #include <FreematicsPlus.h>
// #include <WiFiClientSecure.h>

// #include "abrpwifisecure.h"
// // #include "config.h"

// ABRPWiFiSecure::ABRPWiFiSecure() {
//     client.setCACert(test_root_ca);
// }

// bool ABRPWiFiSecure::start() {
//     WiFi.begin(ssid, password);
// }



// void startWiFi() {
//     Serial.print("Connecting to ");
//     Serial.print(ssid);
//     Serial.print("...");
//     WiFi.begin(ssid, password);
//     while (WiFi.status() != WL_CONNECTED) {
//         Serial.print(".");
//         delay(1000);
//     }
//     Serial.println(" Connected");
//     client.setCACert(test_root_ca);
//     Serial.println("\nStarting connection to server...");
//     if (!client.connect(server, 443))
//         Serial.println("Connection failed!");
//     else {
//         Serial.println("Connected to server!");
//         // Make a HTTP request:
//         client.println("GET https://www.howsmyssl.com/a/check HTTP/1.0");
//         client.println("Host: www.howsmyssl.com");
//         client.println("Connection: close");
//         client.println();

//         while (client.connected()) {
//             String line = client.readStringUntil('\n');
//             if (line == "\r") {
//                 Serial.println("headers received");
//                 break;
//             }
//         }
//         // if there are incoming bytes available
//         // from the server, read them and print them:
//         while (client.available()) {
//             char c = client.read();
//             Serial.write(c);
//         }

//         client.stop();
//     }
// }

// void connectWifi() {
// }
