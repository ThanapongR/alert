import os
import sys
import wave
import struct
import math
import random
import time
import subprocess
import serial

OUTPUT_DIR = "/tmp/fire_alarm_dataset"
SAMPLE_RATE = 16000
PORT = "/dev/cu.usbmodem24101"

os.makedirs(os.path.join(OUTPUT_DIR, "background_noise"), exist_ok=True)
os.makedirs(os.path.join(OUTPUT_DIR, "fire_alarm_t3"), exist_ok=True)
os.makedirs(os.path.join(OUTPUT_DIR, "ambient_beeps"), exist_ok=True)

def write_wav(filename, samples, sample_rate=16000):
    with wave.open(filename, 'w') as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(sample_rate)
        raw = bytearray()
        for s in samples:
            val = max(-32768, min(32767, int(s)))
            raw.extend(struct.pack('<h', val))
        f.writeframes(raw)

print("--- Step 1: Recording 20s In-situ Ambient Noise from INMP441 ---")
try:
    ser = serial.Serial(PORT, 115200, timeout=1)
    time.sleep(1)
    # Drain initial buffer
    ser.reset_input_buffer()
    
    ambient_samples = []
    target_samples = SAMPLE_RATE * 20 # 20 seconds
    
    start_time = time.time()
    while len(ambient_samples) < target_samples and (time.time() - start_time) < 25:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            try:
                val = int(line)
                ambient_samples.append(val)
            except ValueError:
                pass
    ser.close()
    print(f"Recorded {len(ambient_samples)} live samples from INMP441")
    
    # Split into 5-second chunks
    chunk_size = SAMPLE_RATE * 5
    for i in range(0, len(ambient_samples) - chunk_size + 1, chunk_size):
        chunk = ambient_samples[i:i+chunk_size]
        fname = os.path.join(OUTPUT_DIR, "background_noise", f"insitu_room_{i//chunk_size}.wav")
        write_wav(fname, chunk)
except Exception as e:
    print(f"Could not read from serial ({e}), generating synthetic noise fallback")

# Generate additional background noise variations (fan, pink noise, low hum)
print("--- Generating Synthetic Background Noise ---")
for idx in range(10):
    duration_sec = 6.0
    total_samples = int(SAMPLE_RATE * duration_sec)
    samples = []
    # Brownian/Pinkish random walk noise for air conditioner / fan sound
    last_val = 0.0
    for _ in range(total_samples):
        step = random.uniform(-80, 80)
        last_val = 0.95 * last_val + step
        samples.append(last_val + random.gauss(0, 30))
    fname = os.path.join(OUTPUT_DIR, "background_noise", f"synthetic_fan_{idx}.wav")
    write_wav(fname, samples)

print("--- Step 2: Generating Synthetic T3 Fire Alarm Samples ---")
# Temporal Three (T3): 0.5s Beep, 0.5s Silence, 0.5s Beep, 0.5s Silence, 0.5s Beep, 1.5s Silence -> 4.0s cycle
frequencies = [2950, 3000, 3050, 3100, 3150, 3200]

def generate_t3_cycle(freq, volume=16000, duration_cycles=2):
    samples = []
    dt = 1.0 / SAMPLE_RATE
    t = 0.0
    
    # 1 cycle is 4 seconds: 3 beeps + pause
    # Pattern: [ (0.5 on, 0.5 off), (0.5 on, 0.5 off), (0.5 on, 1.5 off) ]
    beeps = [(0.5, 0.5), (0.5, 0.5), (0.5, 1.5)]
    
    for _ in range(duration_cycles):
        for on_dur, off_dur in beeps:
            # ON phase
            on_samples = int(on_dur * SAMPLE_RATE)
            attack_samples = int(0.015 * SAMPLE_RATE) # 15ms ramp
            for s_idx in range(on_samples):
                # Envelope
                if s_idx < attack_samples:
                    env = s_idx / attack_samples
                elif s_idx > on_samples - attack_samples:
                    env = (on_samples - s_idx) / attack_samples
                else:
                    env = 1.0
                
                # Fundamental + 2nd harmonic
                val = env * volume * (
                    0.85 * math.sin(2 * math.pi * freq * t) +
                    0.12 * math.sin(2 * math.pi * (2 * freq) * t) +
                    0.03 * math.sin(2 * math.pi * (3 * freq) * t)
                )
                samples.append(val + random.gauss(0, 50))
                t += dt
            
            # OFF phase (silence with slight ambient noise)
            off_samples = int(off_dur * SAMPLE_RATE)
            for _ in range(off_samples):
                samples.append(random.gauss(0, 50))
                t += dt
    return samples

t3_idx = 0
for freq in frequencies:
    for vol in [10000, 18000, 26000]:
        samples = generate_t3_cycle(freq, volume=vol, duration_cycles=2)
        fname = os.path.join(OUTPUT_DIR, "fire_alarm_t3", f"t3_{freq}hz_vol{vol}_{t3_idx}.wav")
        write_wav(fname, samples)
        t3_idx += 1

print(f"Generated {t3_idx} T3 Fire Alarm samples")

print("--- Step 3: Generating Ambient Non-Fire Beeps ---")
# 1. Microwave / Appliance: 1000Hz or 2000Hz continuous beeps (1s on, 1s off)
# 2. Scanner/Cashier: 2400Hz 80ms short blips
# 3. Truck reverse: 1200Hz 500ms on, 500ms off
# 4. Doorbell / chimes: 2-tone

beep_idx = 0
# Microwave
for m_freq in [1000, 1900, 2000, 2200]:
    for rep in range(3):
        samples = []
        dt = 1.0 / SAMPLE_RATE
        t = 0.0
        for _ in range(3): # 3 long beeps
            for i in range(int(0.8 * SAMPLE_RATE)):
                val = 14000 * math.sin(2 * math.pi * m_freq * t)
                samples.append(val + random.gauss(0, 40))
                t += dt
            for _ in range(int(0.8 * SAMPLE_RATE)):
                samples.append(random.gauss(0, 40))
                t += dt
        fname = os.path.join(OUTPUT_DIR, "ambient_beeps", f"microwave_{m_freq}hz_{beep_idx}.wav")
        write_wav(fname, samples)
        beep_idx += 1

# Scanner short blips
for s_freq in [2300, 2500, 2700]:
    for rep in range(3):
        samples = []
        dt = 1.0 / SAMPLE_RATE
        t = 0.0
        for _ in range(5):
            for i in range(int(0.1 * SAMPLE_RATE)):
                val = 15000 * math.sin(2 * math.pi * s_freq * t)
                samples.append(val + random.gauss(0, 40))
                t += dt
            for _ in range(int(0.9 * SAMPLE_RATE)):
                samples.append(random.gauss(0, 40))
                t += dt
        fname = os.path.join(OUTPUT_DIR, "ambient_beeps", f"scanner_{s_freq}hz_{beep_idx}.wav")
        write_wav(fname, samples)
        beep_idx += 1

# Truck reverse beeps (1200Hz regular 0.5s on, 0.5s off)
for rep in range(4):
    samples = []
    dt = 1.0 / SAMPLE_RATE
    t = 0.0
    for _ in range(6):
        for i in range(int(0.5 * SAMPLE_RATE)):
            val = 16000 * math.sin(2 * math.pi * 1200 * t)
            samples.append(val + random.gauss(0, 40))
            t += dt
        for _ in range(int(0.5 * SAMPLE_RATE)):
            samples.append(random.gauss(0, 40))
            t += dt
    fname = os.path.join(OUTPUT_DIR, "ambient_beeps", f"reverse_1200hz_{beep_idx}.wav")
    write_wav(fname, samples)
    beep_idx += 1

print(f"Generated {beep_idx} ambient beeps samples")

print("--- Step 4: Uploading to Edge Impulse Studio ---")
labels = ["background_noise", "fire_alarm_t3", "ambient_beeps"]
for lbl in labels:
    folder = os.path.join(OUTPUT_DIR, lbl)
    files = [os.path.join(folder, f) for f in os.listdir(folder) if f.endswith('.wav')]
    print(f"Uploading {len(files)} files for label: {lbl} ...")
    cmd = ["edge-impulse-uploader", "--label", lbl, "--category", "split"] + files
    subprocess.run(cmd, check=True)

print("--- ALL DATASETS UPLOADED SUCCESSFULLY! ---")
