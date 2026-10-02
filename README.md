# Acoustic Fire Alarm Detection (ESP32-S3 + INMP441 + Edge Impulse + Home Assistant)

ระบบตรวจจับเสียงสัญญาณเตือนไฟไหม้ (Smoke/Fire Alarm - Temporal Three T3 Pattern) แบบ On-Device TinyML ด้วย **Seeed Studio XIAO ESP32-S3** ร่วมกับไมโครโฟน I2S **INMP441** ประมวลผลผ่านโมเดล **Edge Impulse** และส่งสัญญาณแจ้งเตือนอัตโนมัติเข้าสู่ **Home Assistant** ผ่าน **MQTT Auto-Discovery** พร้อมระบบ Availability (LWT)

---

## 📑 สถาปัตยกรรมและเอกสารกำกับระบบ

- 📖 [GLOSSARY.md](GLOSSARY.md): คำศัพท์และภาษามาตรฐานของระบบ
- 📑 [ADR 0001: MQTT with Home Assistant Discovery and LWT](docs/adr/0001-mqtt-discovery-with-lwt-transport.md)
- 📑 [ADR 0002: Dual-stage State Machine for Alert Verification](docs/adr/0002-dual-stage-alert-verification.md)
- 📑 [ADR 0003: 16kHz MFE Audio Pipeline with PlatformIO Build System](docs/adr/0003-dsp-pipeline-and-platformio.md)
- 📑 [ADR 0004: In-situ Acoustic Data Collection and Compile-time Configuration](docs/adr/0004-data-collection-and-configuration-pattern.md)

---

## 🔌 การต่อสายฮาร์ดแวร์ (Hardware Pinout)

| ขาไมโครโฟน INMP441 | ขาบอร์ด Seeed Studio XIAO ESP32-S3 | หมายเหตุ |
|---|---|---|
| **SCK (BCLK)** | **D8** (GPIO 7) | I2S Bit Clock |
| **WS (LRCK)** | **D9** (GPIO 8) | I2S Word Select |
| **SD (Data Out)**| **D10** (GPIO 9) | I2S Serial Data In |
| **L/R** | **GND** | เลือก Left Channel |
| **VDD** | **3.3V** | แรงดันไฟเลี้ยง 3.3V |
| **GND** | **GND** | กราวด์ร่วม |

*ไฟสถานะ User LED บนบอร์ด Seeed XIAO ESP32-S3 อยู่ที่ **GPIO 21***

---

## 🛠️ ขั้นตอนการพัฒนาและการใช้งาน (End-to-End Workflow)

### ขั้นตอนที่ 1: บันทึกเสียงจริงในสภาพแวดล้อมติดตั้ง (In-situ Data Collection)

1. คอมไพล์และแฟลชเครื่องมือบันทึกเสียง:
   ```bash
   pio run -e data_collector -t upload
   ```
2. ต่อสาย USB ของบอร์ดเข้ากับคอมพิวเตอร์ และรัน Edge Impulse Data Forwarder CLI:
   ```bash
   edge-impulse-data-forwarder --frequency 16000
   ```
3. ล็อกอินบัญชี Edge Impulse และเลือกโปรเจกต์ของคุณ
4. ในหน้า **Data acquisition** ของ Edge Impulse Studio ให้กด Record เสียงบรรยากาศในห้อง (เสียงแอร์, พัดลม, เสียงพูดคุย, ทีวี) กำหนดเป็น label `background_noise` และเสียงเคาะ/ไมโครเวฟเป็น `ambient_beeps`

### ขั้นตอนที่ 2: ตั้งค่าและเทรนโมเดลใน Edge Impulse Studio

1. **Impulse Design**:
   - **Time series data**: Window size `1000 ms`, Window increase `250 ms`, Frequency `16000 Hz`
   - **Processing block**: `Audio (MFE)`
   - **Learning block**: `Classification` (3 คลาส: `fire_alarm_t3`, `ambient_beeps`, `background_noise`)
2. **MFE Settings**:
   - Number of filters: 32
   - Frame length: 0.02
   - Frame stride: 0.01
3. **Classifier Training**:
   - Architecture: 1D หรือ 2D ConvNet (ตาม Default ของ Edge Impulse)
   - Training cycles: 30 - 50 epochs
4. **Deployment**:
   - เลือก **C++ Library** หรือ **Arduino Library**
   - ติ๊กเปิด **EON Compiler** เพื่อประหยัด RAM
   - กด **Build** และดาวน์โหลดไฟล์ `.zip`
   - แตกไฟล์ zip ใส่ไว้ในโฟลเดอร์ `lib/` ของโปรเจกต์นี้

### ขั้นตอนที่ 3: กำหนดค่าเครือข่ายและระบบ

1. คัดลอกไฟล์ `include/secrets.h.example` เป็น `include/secrets.h`:
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```
2. แก้ไขข้อมูล Wi-Fi และ MQTT Broker ของ Home Assistant:
   ```c
   #define WIFI_SSID       "ชื่อไวไฟของคุณ"
   #define WIFI_PASSWORD   "รหัสผ่านไวไฟ"
   #define MQTT_BROKER     "192.168.1.xxx" // IP ของ Home Assistant
   #define MQTT_PORT       1883
   #define MQTT_USER       "mqtt_username"
   #define MQTT_PASSWORD   "mqtt_password"
   ```

### ขั้นตอนที่ 4: แฟลชเฟิร์มแวร์หลัก (Production Firmware)

คอมไพล์และอัปโหลดเฟิร์มแวร์หลัก:
```bash
pio run -e xiao_esp32s3 -t upload -t monitor
```

---

## 🏠 การทำงานร่วมกับ Home Assistant

เมื่อเฟิร์มแวร์เชื่อมต่อ Wi-Fi และ MQTT สำเร็จ จะทำการส่ง **MQTT Auto-Discovery** เข้า Home Assistant โดยอัตโนมัติ:

1. **`binary_sensor.smoke_alarm_acoustic_detector_alarm`**:
   - `device_class`: `smoke`
   - สถานะ: `Clear (OFF)` ปกติ, `Detected (ON)` เมื่อยืนยันพบสัญญาณเตือนไฟไหม้
2. **`sensor.smoke_alarm_acoustic_detector_state`**:
   - สถานะระบบ: `clear` / `suspected` / `alert`
   - Attributes: `confidence`, `consecutive_hits`, `wifi_rssi`, `uptime_sec`, `free_heap`
3. **Availability (LWT)**:
   - หากไฟดับหรือบอร์ดหลุดการเชื่อมต่อ Entity ใน Home Assistant จะขึ้นสถานะ `Unavailable` ทันที

### ตัวอย่าง Home Assistant Automation (automations.yaml)

```yaml
alias: "Emergency: Smoke Alarm Acoustic Trigger"
description: "เมื่อตรวจพบเสียงสัญญาณไฟไหม้ ให้เปิดไฟทั้งบ้านและแจ้งเตือนด่วน"
trigger:
  - platform: state
    entity_id: binary_sensor.smoke_alarm_acoustic_detector_alarm
    to: "on"
action:
  # 1. เปิดไฟสว่างทั้งบ้าน
  - service: light.turn_on
    target:
      entity_id: all
    data:
      brightness_pct: 100
  # 2. ส่ง Notification แจ้งเตือนฉุกเฉินเข้ามือถือ
  - service: notify.notify
    data:
      title: "🚨 สัญญาณเตือนไฟไหม้ดังขึ้น!"
      message: "ระบบตรวจพบเสียงเตือนไฟไหม้ T3 ต่อเนื่อง กรุณาตรวจสอบทันที"
      data:
        push:
          sound: "emergency"
          critical: 1
mode: single
```

---

## 💡 สัญญาณไฟ LED บนบอร์ด (GPIO 21)

- **กะพริบช้า (1 Hz)**: กำลังเชื่อมต่อ Wi-Fi / MQTT
- **ดับ (มี Heartbeat กระพริบสั้นทุก 5 วิ)**: สถานะ `Clear` (ทำงานปกติ ฟังเสียงตลอด 24/7)
- **กะพริบเร็ว (5 Hz)**: สถานะ `Suspected` (ตรวจพบเสียงบี๊บ 1 ครั้ง กำลังเฝ้ารอเพื่อยืนยัน)
- **ติดสว่างค้าง**: สถานะ `Alert` (ยืนยันเสียงไฟไหม้ต่อเนื่อง และส่ง Alert ไปยัง HA แล้ว)
