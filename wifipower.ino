#include <Ticker.h>
#include "RtcDS1302.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
// Data for access point
const char *ssid = "Rele";
const char *password = "rele2205";
byte data[5]; // // stors data from EEPROM
volatile boolean check = false; // This flag indecates that we need to check our RTC
Ticker tk;
ESP8266WebServer server(80); // is an object for web server
 RtcDS1302 rtc(D7, D6, D5); // is An object for RTC
// Constants
#define RELAY D4 // relay is connected to digital pin 4
#define LED 13 // building led is connected to digital pin 13. (Led was connected only for debugging, in prodaction version you will not see it)
#define ON 0
#define OFF 1
// http handlers
void getSchedulerConfiguration() { // returns scheduler's configuration directly from EEPROM
		for (int i = 0; i <= 4; i++) {
		data[i] = EEPROM.read(i);
	}
	String reply = "{\"startHour\":";
	reply += String(data[0]);
	reply += ",";
	reply += "\"startMinute\":";
	reply += String(data[1]);
	reply += ",";
	reply += "\"endHour\":";
	reply += String(data[2]);
	reply += ",";
	reply += "\"endMinute\":";
	reply += String(data[3]);
	reply += ",";
	reply += "\"controleSumm\":";
	reply += String(data[4]);
	reply += "}";
	server.send(200, "application/json", reply);
}

void setTime() {
	int year = atoi(server.arg("year").c_str());
	int month = atoi(server.arg("month").c_str());
	int day = atoi(server.arg("day").c_str());
	int hour = atoi(server.arg("hour").c_str());
	int minute = atoi(server.arg("minute").c_str());
	rtc.setYear(year);
	rtc.setMonth(month);
	rtc.setDay(day);
	rtc.setHour(hour);
	rtc.setMinute(minute);
	server.send(200);
}
void getState() { // returns state of your relay
	if (digitalRead(D4) == ON) server.send(200, "application/json", "{\"state\":1}");
	else server.send(200, "application/json", "{\"state\":0}");
}

void handleRoot() { // shows the main page
	String mainPage; // // keeps HTML output for main page
	File f = SPIFFS.open("/index.htm", "r");
	mainPage = f.readString();
	server.send(200, "text/html", mainPage);
}
void switchRelay() {
	digitalWrite(D4, !digitalRead(D4));
	server.send(200);
}
void getTime() {
	String reply = String(rtc.getHour());
	reply += ":";
	if (rtc.getMinute() < 10) reply += "0";
	reply += String(rtc.getMinute());
	server.send(200, "text", reply);
}
void configSchaduler() {
	int startHour = atoi(server.arg("startHour").c_str());
	int startMinute = atoi(server.arg("startMinute").c_str());
	int endHour = atoi(server.arg("endHour").c_str());
	int endMinute = atoi(server.arg("endMinute").c_str());
	EEPROM.write(0, startHour);
	EEPROM.write(1, startMinute);
	EEPROM.write(2, endHour);
	EEPROM.write(3, endMinute);
	int summ = startHour + startMinute + endHour + endMinute; // Controle summ
	EEPROM.write(4, summ);
	EEPROM.commit();
	server.send(200);
}
void setup() {
	delay(1000);
	pinMode(D4, OUTPUT);
	digitalWrite(D4, OFF);
	Serial.begin(9600);
	Serial.println();
	Serial.print("Configuring access point...");
	/// You can remove the password parameter if you want the AP to be open.
	WiFi.softAP(ssid, password);
SPIFFS.begin();
	server.on("/", handleRoot);
	server.on("/switch", switchRelay);
server.on("/state", getState);
	server.on("/config/scheduler", configSchaduler);
	server.on("/scheduler", getSchedulerConfiguration);
	server.on("/time/get", getTime);
	server.on("/time/set", setTime);
	server.begin();
	Serial.println("HTTP server started");
	EEPROM.begin(512);
	// Configuring RTC
	rtc.begin();
	tk.attach(5, ISRTimer); // Every 30 seconds interruption will be generating. DSee function 'ISRTimer'
}
void loop() {
	server.handleClient();
	if (check) {
		check = false;
		for (int i = 0; i <= 4; i++) {
			data[i] = EEPROM.read(i);
		}
		if (data[0] + data[1] + data[2] + data[3] != data[4]) Serial.println("Checksumm error");
		if (rtc.getHour() == data[0] && rtc.getMinute() == data[1]) digitalWrite(D4, ON);
		if (rtc.getHour() >= data[2] && rtc.getMinute() >= data[3]) digitalWrite(D4, OFF);
	}
}

// interruption handlers
void ISRTimer() {
	check = true;
}