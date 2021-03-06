/*
###############################################################################
#
# Temboo Arduino library
#
# Copyright 2016, Temboo Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###############################################################################
*/


#include <Arduino.h>
#include <Mailbox.h>
#include "utility/TembooGlobal.h"
#include "TembooMonitoring.h"

const int MAX_MAILBOX_MESSAGE_SIZE = 128;

const unsigned long POLL_TIMEOUT = 50;
const unsigned long INITIATE_TIMEOUT_MS = 5000;
const unsigned long MCU_PING_PERIOD_MS = 5000;

void logTembooDebug(const char *c) {
	Console.print("Debug: ");
    Console.println(c);
}

void addWebSocketPinData(int pin, int pinVal, bool requestResponse) {
	TembooMessaging::sendData(pin, pinVal, requestResponse);
}

void updateIntervalTime(int intervalTime) {
	uint8_t msg[MAX_MAILBOX_MESSAGE_SIZE] = {0};
    int messageSize = snprintf((char*)msg, MAX_MAILBOX_MESSAGE_SIZE, "Mi|V%i", intervalTime);
    Mailbox.writeMessage(msg, messageSize);
}

static void tembooDigitalWrite(uint32_t pin, int val) {
	digitalWrite(pin, val);
}

uint32_t tembooDigitalRead(uint32_t pin) {
	return digitalRead(pin);
}

static void tembooAnalogWrite(uint32_t pin, int val) {
	analogWrite(pin, val);
}

uint32_t tembooAnalogRead(uint32_t pin) {
	return analogRead(pin);
}

TembooMessaging::TembooMessaging(TembooPinTable* pinTable, int pinTableSize) {
	m_accountName = NULL;
	m_appKey = NULL;
	m_appKeyName = NULL;
	m_deviceID = NULL;
	m_connectionStatus = false;
	m_pinTable = pinTable;
	m_pinTableSize = pinTableSize;
	m_pinTableDepth = 0;
	m_connectionAttemptTime = millis();
	m_lastPingTime = millis();
	// init array
	int i = 0;
	for (i = 0; i < m_pinTableSize; i++) {
		m_pinTable[i].pin = -1;
		m_pinTable[i].pinRead = NULL;
		m_pinTable[i].pinWrite = NULL;
	}
}

void TembooMessaging::startMessaging() {
	if (!running()) {
		TEMBOO_TRACELN("starting messanger");
		m_connectionStatus = false;
		Process::begin("tembooMessaging");
		runAsynchronously();
	}
}

void TembooMessaging::setPinsToDefaultState() {
    int i = 0;
    for (; i < m_pinTableDepth; i++) {
        if (m_pinTable[i].pinWrite != NULL) {
            m_pinTable[i].pinWrite(m_pinTable[i].pin, m_pinTable[i].defaultValue);
            m_pinTable[i].currentPinValue = m_pinTable[i].defaultValue;
        }
    }
}

void TembooMessaging::addPinConfig(int pin, const char* type, int mode, int defaultValue) {
    // check if there is room to add new pin information
    if (m_pinTableDepth == m_pinTableSize) {
        return;
    }
    bool success = true;
    // check if pin has already been added, if it has
    // edit the config/mode, else, add to config
    int i = 0;
    for (; i < m_pinTableDepth; i++) {
        if (m_pinTable[i].pin == pin) {
            break;
        }
    }
    if (mode == INPUT) {
        if (!strcasecmp(type, "analog")) {
            pinMode(pin, INPUT);
            m_pinTable[i].pinRead = tembooAnalogRead;
            m_pinTable[i].pinWrite = NULL;
            m_pinTable[i].pin = pin;
            m_pinTable[i].currentPinValue = tembooAnalogRead(pin);
            m_pinTable[i].defaultValue = defaultValue;
        } else if (!strcasecmp(type, "digital")) {
            pinMode(pin, INPUT);
            m_pinTable[i].pinRead = tembooDigitalRead;
            m_pinTable[i].pinWrite = NULL;
            m_pinTable[i].pin = pin;
            m_pinTable[i].currentPinValue = tembooDigitalRead(pin);
            m_pinTable[i].defaultValue = defaultValue;
        } else {
            success = false;
        }
    } else if (mode == OUTPUT) {
        if (!strcasecmp(type, "analog")) {
            pinMode(pin, OUTPUT);
            m_pinTable[i].pinRead = NULL;
            m_pinTable[i].pinWrite = tembooAnalogWrite;
            m_pinTable[i].pin = pin;
            m_pinTable[i].currentPinValue = defaultValue;
            m_pinTable[i].defaultValue = defaultValue;
            tembooAnalogWrite(pin, defaultValue);
        } else if (!strcasecmp(type, "digital")) {
            pinMode(pin, OUTPUT);
            m_pinTable[i].pinRead = NULL;
            m_pinTable[i].pinWrite = tembooDigitalWrite;
            m_pinTable[i].pin = pin;
            m_pinTable[i].currentPinValue = defaultValue;
            m_pinTable[i].defaultValue = defaultValue;
            tembooDigitalWrite(pin, defaultValue);
        } else {
            success = false;
        }
    } else {
        success = false;
    }
    if (success) {
        if (i >= m_pinTableDepth) {
            // if we needed to add a new pin, increment depth
            m_pinTableDepth++;
        }
    }
}

void TembooMessaging::begin() {
	Mailbox.begin();
	startMessaging();
}

void TembooMessaging::setAccountName(const String& accountName) {
	m_accountName = accountName.c_str();
}
void TembooMessaging::setAccountName(const char* accountName) {
	m_accountName = accountName;
}

void TembooMessaging::setAppKeyName(const String& appKeyName) {
	m_appKeyName = appKeyName.c_str();
}
void TembooMessaging::setAppKeyName(const char* appKeyName) {
	m_appKeyName = appKeyName;
}

void TembooMessaging::setAppKey(const String& appKey) {
	m_appKey = appKey.c_str();
}
void TembooMessaging::setAppKey(const char* appKey) {
	m_appKey = appKey;
}

void TembooMessaging::setDeviceID(const String& deviceID) {
	m_deviceID = deviceID.c_str();
}
void TembooMessaging::setDeviceID(const char* deviceID) {
	m_deviceID = deviceID;
}

void TembooMessaging::initiateConnection() {
	unsigned long now = millis();
	if (now - m_connectionAttemptTime < INITIATE_TIMEOUT_MS) {
		poll();
		return;
	}
	if (m_accountName == NULL || *m_accountName == '\0') {
		logTembooDebug("Account name invalid or not set");
		return;
	}
	if (m_appKeyName == NULL || *m_appKeyName == '\0') {
		logTembooDebug("Appkey name invalid or not set");
		return;
	}
	if (m_deviceID == NULL || *m_deviceID == '\0') {
		logTembooDebug("DeviceID invalid or not set");
		return;
	}
	if (m_appKey == NULL || *m_appKey == '\0') {
		logTembooDebug("Appkey invalid or not set");
		return;
	}
	startMessaging();
	int messageSize = strlen(m_accountName) + strlen(m_appKey) + strlen(m_appKeyName) + strlen(m_deviceID) + 11;
	uint8_t msg[messageSize];
	if (messageSize < MAX_MAILBOX_MESSAGE_SIZE) {
		Console.println("Attempting to connect to Temboo");
		messageSize = snprintf((char*)msg, messageSize, "MI|N%s|K%s|B%s|A%s", m_accountName, m_appKeyName, m_deviceID, m_appKey);
		Mailbox.writeMessage(msg, messageSize);
		m_connectionAttemptTime = now;
	} else {
		logTembooDebug("Initiation request too large");
		return;
	}
}

WSMessageRequest TembooMessaging::poll() {
	startMessaging();
	long int now = millis();
	WSMessageRequest rc = WS_NO_MESSAGE;
	while (millis() - now < POLL_TIMEOUT) {
		if (millis() - m_lastPingTime >= MCU_PING_PERIOD_MS) {
            m_lastPingTime = millis();
            sendPing();
        }
    	if (Mailbox.messageAvailable()) {
    		uint8_t msg[MAX_MAILBOX_MESSAGE_SIZE] = {0};
    		int recvLen = Mailbox.readMessage(msg, MAX_MAILBOX_MESSAGE_SIZE);
    		if (recvLen > 0) {
	    		rc = handleResponse(msg, m_pinTable, m_pinTableDepth, m_connectionStatus);
	    		if (rc == WS_UPDATE_CONNECTED) {
	    			logTembooDebug("Connected to Temboo");
	    			m_connectionStatus = true;
	    		} else if (rc == WS_UPDATE_DISCONNECTED) {
	    			logTembooDebug("Disconnected from Temboo");
	    			m_connectionStatus = false;
	    		} else if (rc == WS_REQUEST_ERROR) {
	    			// disconnect
	    			sendError("Message request error");
	    		}
	    	}
    	}
    }
    return rc;
}

void TembooMessaging::updatePinValue(int pinNum, int pinVal) {
	// save the data to the strcuture and then send to Temboo
	int i = 0;
    for (; i < m_pinTableDepth; i++) {
    	if (m_pinTable[i].pin == pinNum) {
    		m_pinTable[i].currentPinValue = pinVal;
    		// if pin has pinWrite as NULL, it is an input
    		// pin and needs to be stored. If not NULL,
    		// pin is an actuator and should not be stored
    		if(m_pinTable[i].pinWrite == NULL){
    			sendData(pinNum, pinVal, false);
    		} else {
    			sendData(pinNum, pinVal, true);
    		}
    		return;
    	}
    }
    logTembooDebug("Pin not found, unable to update");
}

int TembooMessaging::retrievePinValue(int pinNum) {
	// search through pin structure and return the pin value
	int i = 0;
    for (; i < m_pinTableDepth; i++) {
    	if (m_pinTable[i].pin == pinNum) {
    		return m_pinTable[i].currentPinValue;
    	}
    }
    logTembooDebug("Pin not found, unable to obtain value");
    return 0;
}

void TembooMessaging::sendError(const char* errorText) {
	uint8_t msg[MAX_MAILBOX_MESSAGE_SIZE] = {0};
    int messageSize = snprintf((char*)msg, MAX_MAILBOX_MESSAGE_SIZE, "ME|T%s", errorText);
    Mailbox.writeMessage(msg, messageSize);
}

void TembooMessaging::disconnectWSConnection(int closeCode, const char* closeReason) {
	int messageSize = strlen(closeReason) + 11;
	uint8_t msg[messageSize];
	messageSize = snprintf((char*)msg, messageSize, "MF|O%i|r%s", closeCode, closeReason);
	Mailbox.writeMessage(msg, messageSize);
}

void TembooMessaging::sendData(int pin, int pinVal, bool requestResponse) {
    uint8_t msg[MAX_MAILBOX_MESSAGE_SIZE] = {0};
    int messageSize = snprintf((char*)msg, MAX_MAILBOX_MESSAGE_SIZE, "MD|P%i|V%i%s", pin, pinVal, requestResponse ? "|Q" : "");
    Mailbox.writeMessage(msg, messageSize);
}
void TembooMessaging::sendData(int pin, float pinVal) {
	uint8_t msg[MAX_MAILBOX_MESSAGE_SIZE] = {0};
	char floatStr[12] = {0};
	dtostrf(pinVal, 4, 2, floatStr);
	int messageSize = snprintf((char*)msg, MAX_MAILBOX_MESSAGE_SIZE, "MD|P%i|V%s", pin, floatStr);
	Mailbox.writeMessage(msg, messageSize);
}

bool TembooMessaging::isConnected() {
	if (running()) {
    	return m_connectionStatus;
    }
    return false;
}

void TembooMessaging::sendPing() {
    Mailbox.writeMessage((uint8_t*)"P",1);
}
