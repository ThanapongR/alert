# Fire Alarm Acoustic Detection Context

ระบบตรวจจับเสียงสัญญาณเตือนไฟไหม้ (Smoke/Fire Alarm) ด้วย ESP32-S3 และไมโครโฟน I2S INMP441 ประมวลผลด้วย Edge Impulse TinyML และส่งสัญญาณแจ้งเตือนไปยัง Home Assistant ผ่าน MQTT

## Language

**Target Alarm**:
สัญญาณเสียงเตือนไฟไหม้ที่ระบบต้องการตรวจจับ โดยเฉพาะสัญญาณมาตรฐานแบบ Temporal-Three (T3 Pattern) ความถี่หลัก ~3 kHz
_Avoid_: Siren, buzz, noise

**Temporal-Three (T3)**:
รูปแบบสัญญาณเสียงเตือนภัยมาตรฐานสากล (ISO 8201 / NFPA 72) ประกอบด้วยเสียงบี๊บ 3 ครั้งสั้นๆ แล้วหยุดพัก (0.5s on, 0.5s off x 3 ครั้ง แล้วหยุด 1.5s รวมรอบละ 4 วินาที)
_Avoid_: Beep beep, melody

**Dual-stage State Machine**:
กลไกสถานะการยืนยันสัญญาณเตือนภัยบน ESP32 แบ่งเป็น Clear (ปกติ), Suspected (ตรวจพบเบื้องต้น), และ Alert (ยืนยันพบเหตุต่อเนื่อง) เพื่อป้องกัน False Alarm
_Avoid_: Debounce filter, delay timer

**Home Assistant MQTT Discovery**:
กลไกที่ ESP32 ส่ง Configuration payload ไปยัง Home Assistant ผ่าน MQTT อัตโนมัติ เพื่อสร้างและผูก Entity โดยไม่ต้อง config ฝั่ง HA ด้วยตนเอง
_Avoid_: Webhook trigger, polling API

**Dual Entity**:
รูปแบบการนำเสนออุปกรณ์ใน Home Assistant ที่ประกอบด้วย `binary_sensor` สำหรับเตือนภัยไฟไหม้ (smoke device class) และ `sensor` สำหรับมอนิเตอร์สถานะภายใน (`clear`, `suspected`, `alert`) พร้อมเมทริกซ์การตรวจจับ
_Avoid_: Single sensor, raw state string

**Mel-Filterbank Energy (MFE)**:
บล็อกประมวลผลสัญญาณดิจิทัล (DSP) ที่แปลงสัญญาณเสียง 16 kHz เป็นสเปกตรัมความถี่โดยใช้ Mel scale เหมาะกับการตรวจจับเสียงสังเคราะห์และสัญญาณเตือนภัยที่มีโทนเสียงแหลมเฉพาะตัว
_Avoid_: MFCC, raw waveform

**XIAO I2S Bus**:
การเชื่อมต่อสายสัญญาณเสียง I2S ระหว่าง Seeed Studio XIAO ESP32-S3 กับ INMP441 กำหนดใช้ SCK->D8 (GPIO 7), WS->D9 (GPIO 8), SD->D10 (GPIO 9)
_Avoid_: Analog ADC, PDM mic

**Availability (LWT)**:
กลไก Last Will and Testament ของ MQTT เพื่อระบุสถานะ Online/Offline ของอุปกรณ์ตัวตรวจจับให้ Home Assistant รับรู้ทันทีหากไฟดับหรือเน็ตหลุด
_Avoid_: Ping, heartbeat poll

**In-situ Data Collection**:
การเก็บข้อมูลเสียงตัวอย่างสภาพแวดล้อมจริงผ่านไมโครโฟน INMP441 และ ESP32-S3 ตัวจริงเพื่อนำไปเทรนโมเดลใน Edge Impulse ทำให้ Frequency Response และ Noise Floor ตรงกับสภาวะรันจริง
_Avoid_: Smartphone recording, generic web samples

**Status LED**:
ไฟ LED บนบอร์ด (GPIO 21) สำหรับแสดงสถานะของระบบแบบ Visual Feedback แบ่งตามจังหวะการกะพริบตาม State Machine
_Avoid_: Indicator lamp, blinker
