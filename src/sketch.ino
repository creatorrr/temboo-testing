#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Temboo.h>
#include "TembooAccount.h" // Contains Temboo account information

WiFiClient client;

int numRuns = 1;   // Execution count, so this doesn't run forever
int maxRuns = 10;   // Maximum number of times the Choreo should be executed

void setup() {
  Serial.begin(9600);
  
  // For debugging, wait until the serial console is connected
  delay(4000);
  while(!Serial);

  int wifiStatus = WL_IDLE_STATUS;

  // Determine if the WiFi Shield is present
  Serial.print("\n\nShield:");
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("FAIL");

    // If there's no WiFi shield, stop here
    while(true);
  }

  Serial.println("OK");

  // Try to connect to the local WiFi network
  while(wifiStatus != WL_CONNECTED) {
    Serial.print("WiFi:");
    wifiStatus = WiFi.begin(WIFI_SSID, WEP_KEY_INDEX, WEP_KEY);
    if (wifiStatus == WL_CONNECTED) {
      Serial.println("OK");
    } else {
      Serial.println("FAIL");
    }
    delay(5000);
  }

  Serial.println("Setup complete.\n");
}

void loop() {
  if (numRuns <= maxRuns) {
    Serial.println("Running SendMessage - Run #" + String(numRuns++));

    TembooChoreo SendMessageChoreo(client);

    // Invoke the Temboo client
    SendMessageChoreo.begin();

    // Set Temboo account credentials
    SendMessageChoreo.setAccountName(TEMBOO_ACCOUNT);
    SendMessageChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    SendMessageChoreo.setAppKey(TEMBOO_APP_KEY);

    // Set profile to use for execution
    SendMessageChoreo.setProfile("NEXMO");

    // Identify the Choreo to run
    SendMessageChoreo.setChoreo("/Library/Nexmo/SMS/SendMessage");

    // Run the Choreo; when results are available, print them to serial
    SendMessageChoreo.run();

    while(SendMessageChoreo.available()) {
      char c = SendMessageChoreo.read();
      Serial.print(c);
    }
    SendMessageChoreo.close();
  }

  Serial.println("\nWaiting...\n");
  delay(30000); // wait 30 seconds between SendMessage calls
}