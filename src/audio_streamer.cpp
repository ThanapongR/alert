#include "audio_streamer.h"
#include "config.h"

AudioStreamer audio_streamer(8080);

struct __attribute__((packed)) WavHeader {
    char riff_tag[4];        // "RIFF"
    uint32_t riff_size;      // 0x7FFFFFF0
    char wave_tag[4];        // "WAVE"
    char fmt_tag[4];         // "fmt "
    uint32_t fmt_size;       // 16
    uint16_t audio_format;   // 1 (PCM)
    uint16_t num_channels;   // 1 (Mono)
    uint32_t sample_rate;    // 16000
    uint32_t byte_rate;      // 32000 (16000 * 1 * 2)
    uint16_t block_align;    // 2 (1 * 2)
    uint16_t bits_per_sample;// 16
    char data_tag[4];        // "data"
    uint32_t data_size;      // 0x7FFFFFF0
};

AudioStreamer::AudioStreamer(uint16_t port)
    : server_port(port),
      server(port),
      client_connected(false) {}

void AudioStreamer::begin() {
    server.begin();
    Serial.printf("[Streamer] Live Audio Server started on port %u (path: /stream.wav)\n", server_port);
}

void AudioStreamer::sendHeaders() {
    // Send HTTP/1.1 200 OK headers
    client.print("HTTP/1.1 200 OK\r\n");
    client.print("Content-Type: audio/wav\r\n");
    client.print("Access-Control-Allow-Origin: *\r\n");
    client.print("Cache-Control: no-cache, no-store, must-revalidate\r\n");
    client.print("Connection: close\r\n\r\n");

    // Send 44-byte WAV header for continuous live PCM stream
    WavHeader header;
    memcpy(header.riff_tag, "RIFF", 4);
    header.riff_size = 0x7FFFFFF0;
    memcpy(header.wave_tag, "WAVE", 4);
    memcpy(header.fmt_tag, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = 1; // Mono
    header.sample_rate = AUDIO_SAMPLE_RATE; // 16000
    header.byte_rate = AUDIO_SAMPLE_RATE * sizeof(int16_t); // 32000
    header.block_align = sizeof(int16_t); // 2
    header.bits_per_sample = 16;
    memcpy(header.data_tag, "data", 4);
    header.data_size = 0x7FFFFFF0;

    client.write((const uint8_t*)&header, sizeof(WavHeader));
}

void AudioStreamer::update() {
    // Check for incoming client connection
    if (server.hasClient()) {
        WiFiClient new_client = server.available();
        if (new_client) {
            if (client_connected && client.connected()) {
                // If a client is already connected, drop previous or replace
                client.stop();
            }

            client = new_client;
            client_connected = true;

            // Consume incoming HTTP GET request headers
            unsigned long start_wait = millis();
            while (client.connected() && !client.available() && millis() - start_wait < 500) {
                delay(1);
            }
            while (client.available()) {
                String line = client.readStringUntil('\n');
                if (line == "\r" || line.length() == 0) break;
            }

            Serial.printf("[Streamer] Client connected from %s! Streaming live audio...\n", 
                          client.remoteIP().toString().c_str());
            sendHeaders();
        }
    }

    // Check if current client disconnected
    if (client_connected && !client.connected()) {
        client.stop();
        client_connected = false;
        Serial.println("[Streamer] Client disconnected. Stream stopped.");
    }
}

void AudioStreamer::broadcastAudio(const int16_t* samples, size_t count) {
    if (!client_connected) return;

    if (!client.connected()) {
        client.stop();
        client_connected = false;
        Serial.println("[Streamer] Client disconnected. Stream stopped.");
        return;
    }

    // Send PCM chunk directly over TCP
    client.write((const uint8_t*)samples, count * sizeof(int16_t));
}
