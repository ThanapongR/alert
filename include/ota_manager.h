#pragma once

#include <Arduino.h>

class OtaManager {
public:
    OtaManager();
    void begin();
    void update();
    bool isUpdating() const { return updating; }

private:
    bool initialized;
    bool updating;
};

extern OtaManager ota_manager;
