# 2. Dual-stage State Machine for Acoustic Alert Verification

We decided to process Edge Impulse inference results through a 3-state machine (Clear -> Suspected -> Alert) rather than triggering alerts on a single high-confidence inference window. High-pitch sounds like microwave timers, TV shows, and vehicle horns can briefly spike classification confidence; by requiring consecutive verified detections across a debounce window before latching into an Alert state, we trade 3-5 seconds of detection latency for dramatic reduction in false positives that would otherwise cause alarm fatigue.
