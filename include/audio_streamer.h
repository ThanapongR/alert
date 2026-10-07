#pragma once

#include <Arduino.h>
#include <WiFi.h>

class AudioStreamer {
public:
    AudioStreamer(uint16_t port = 8080);

    void begin();
    void update();
    void broadcastAudio(const int16_t* samples, size_t count);
    bool hasClient() const { return client_connected; }

private:
    uint16_t server_port;
    WiFiServer server;
    WiFiClient client;
    bool client_connected;

    void sendHeaders();
};

extern AudioStreamer audio_streamer;
