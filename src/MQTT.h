#pragma once
#include <Arduino.h>

// Returns true if the broker accepts a connection with the given credentials.
// Pass an empty user string for anonymous connect.
bool testMqttConnection(const String& broker, uint16_t port,
                        const String& user, const String& pass);

// Connect to broker, subscribe to command topics, publish HA Discovery + state.
// Call once after WiFi connects when mqttEnabled is true.
void initMqtt();

// Non-blocking keep-alive: call every loop() iteration.
// Reconnects automatically after disconnection (5 s back-off).
void loopMqtt();

// Publish current device state to the MQTT state topic.
// Call after any local change (brightness, layout) so HA stays in sync.
void publishMqttState();
