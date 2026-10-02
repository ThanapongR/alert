# 1. Use MQTT with Home Assistant Discovery and LWT for Alert Transport

For the fire alarm acoustic detection system, we chose MQTT with Home Assistant Auto-Discovery and Last Will and Testament (LWT) over direct HTTP Webhooks or REST API calls. Life-safety acoustic detection requires Home Assistant to know not only when an alarm occurs, but crucially when the detector node goes offline (power failure or Wi-Fi disconnection). MQTT LWT provides immediate offline status monitoring without polling overhead, while Auto-Discovery enables zero-configuration entity setup in Home Assistant.
