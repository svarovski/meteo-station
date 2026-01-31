# ESP8266 Pin Mapping Reference

## NodeMCU/Wemos D1 Mini Pin Labels

| NodeMCU | Wemos D1 | GPIO | Function in Project |
|---------|----------|------|---------------------|
| D0      | D0       | 16   | AHT10 Power Control (must connect to RST for wake) |
| D1      | D1       | 5    | I2C SCL (AHT10)     |
| D2      | D2       | 4    | I2C SDA (AHT10)     |
| D3      | D3       | 0    | Boot/Flash Button (Wake button) |
| D4      | D4       | 2    | Built-in LED (optional indicator) |
| A0      | A0       | ADC  | Battery Voltage Monitor |
| RST     | RST      | RST  | Reset (connect to button + GPIO16) |
| 3V3     | 3V3      | -    | Power Output (from AMS1117) |
| GND     | GND      | -    | Ground               |

## Critical Connections

### 1. Deep Sleep Wake Circuit
```
GPIO16 (D0) ───[220Ω]─┬─── RST
                      │
                   [10kΩ]
                      │
                     3.3V
```
The resistor limits current during wake pulse. Pull-up ensures stable reset.

### 2. Battery Voltage Divider
```
Battery+ ───[1MΩ]─┬─── A0
                  │
               [330kΩ]
                  │
                 GND
```

**Calculation**:
- Battery max: 4.2V
- Voltage at A0: 4.2V × (330kΩ / (1MΩ + 330kΩ)) = 1.04V
- ESP8266 ADC max input (with internal divider): ~3.3V (but best below 1V)
- With internal 220kΩ/100kΩ divider: max ~1V external input

**Alternative (safer)**:
```
Battery+ ───[470kΩ]─┬─── A0
                    │
                 [100kΩ]
                    │
                   GND
```
This gives 0.74V at A0 when battery is 4.2V.

### 3. I2C Pull-ups (Critical for AHT10)
```
3.3V ───[4.7kΩ]─┬─── SDA (GPIO4)
                │
              AHT10
                
3.3V ───[4.7kΩ]─┬─── SCL (GPIO5)
                │
              AHT10
```

## Power Architecture

```
┌─────────────┐
│ 18650 Cell  │
│ 2.7V - 4.2V │
└──────┬──────┘
       │
       ├─────[1MΩ]────┬── A0 (Voltage Monitor)
       │              │
       │           [330kΩ]
       │              │
       │             GND
       │
       ▼
┌─────────────┐
│  AMS1117    │ (or better: HT7333/MCP1700)
│  3.3V LDO   │
└──────┬──────┘
       │
       ├────── ESP8266 VCC
       ├────── ESP8266 CH_PD/EN
       └────── (GPIO16 powers AHT10 dynamically)
```

## GPIO States During Operation

| Mode          | GPIO16 (AHT Pwr) | GPIO4 (SDA) | GPIO5 (SCL) | GPIO0 (Btn) |
|---------------|------------------|-------------|-------------|-------------|
| Deep Sleep    | LOW (sensor off) | -           | -           | HIGH (pull-up) |
| Measuring     | HIGH (sensor on) | I2C Data    | I2C Clock   | HIGH        |
| Config Mode   | LOW              | -           | -           | LOW (pressed) |
| WiFi Upload   | LOW              | -           | -           | -           |

## ESP8266 Boot Mode Selection

**GPIO0 (D3) during boot determines mode:**
- HIGH (button NOT pressed): Normal boot → Run program
- LOW (button pressed): Flash mode → Programming

**GPIO15 must be LOW, GPIO2 must be HIGH** during boot:
- Most dev boards handle this automatically
- If using bare ESP-12, ensure proper pull-ups/downs

## Current Draw by State

| State              | Current  | Duration         | Notes                    |
|--------------------|----------|------------------|--------------------------|
| Deep Sleep         | ~20µA    | Most of time     | ESP8266 + HT7333         |
| Wake & Measure     | ~80mA    | 3-5 seconds      | Includes sensor reading  |
| WiFi Connect       | ~120mA   | 5-10 seconds     | Peak ~170mA              |
| WiFi Transmit      | ~100mA   | 2-5 seconds      | Depends on data size     |
| Config Mode (AP)   | ~80mA    | User-dependent   | Continuous until config  |

## Troubleshooting with Multimeter

### Check Points:
1. **Battery Voltage**: Should be 3.0V - 4.2V
2. **3.3V Rail**: Should be stable 3.3V ±0.1V
3. **GPIO16 to RST**: Should show pulse when waking (use oscilloscope)
4. **A0 Voltage**: Should be 0.6V - 1.0V (proportional to battery)
5. **I2C Lines (idle)**: Should be pulled HIGH (~3.3V)
6. **Deep Sleep Current**: Should be <100µA total

### Common Issues:
- **ESP won't wake**: No connection GPIO16↔RST, or missing pull-up on RST
- **I2C fails**: Missing pull-ups on SDA/SCL
- **High sleep current**: Check AMS1117 (use HT7333 instead)
- **ADC reads 0 or 1023**: Wrong voltage divider or damaged ADC

## Flash Programming Connections

**Using USB-Serial adapter:**
```
Adapter    ESP8266
-------    -------
TX    →    RX
RX    ←    TX
3.3V  →    VCC
GND   →    GND

For programming:
- Hold GPIO0 (D3) to GND
- Press Reset
- Release GPIO0
- Upload sketch
```

**Important**: Most NodeMCU/Wemos boards have built-in USB-Serial, making manual connections unnecessary.

## Additional Hardware Recommendations

### 1. Power Stability
- Add 100µF electrolytic capacitor across battery terminals
- Add 10µF ceramic capacitor at ESP8266 VCC pin
- Add 100nF ceramic capacitor at AHT10 VCC pin

### 2. Protection
- Use 18650 with **built-in protection circuit**
- Add reverse polarity protection (Schottky diode or P-MOSFET)
- ESD protection on external connections (optional)

### 3. Weatherproofing (Outdoor Use)
- IP65+ rated enclosure
- Cable glands for any external connections
- Desiccant packet inside enclosure
- Conformal coating on PCB (optional)

### 4. Expansion Headers (Future Proofing)
Leave broken out:
- D5, D6, D7 (GPIO14, 12, 13) for SPI or additional sensors
- 3.3V and GND test points
- I2C headers (already in use, but add extra connector)

## Testing Checklist

Before deployment:
- [ ] Verify all voltages with multimeter
- [ ] Test deep sleep current (<100µA)
- [ ] Confirm sensor readings are accurate
- [ ] Test button wake and long-press config
- [ ] Verify WiFi connection and InfluxDB upload
- [ ] Check battery life calculation (24h test at interval)
- [ ] Verify NTP sync works
- [ ] Test with fully charged and low battery
- [ ] Check enclosure is sealed (if outdoor)

## Component Substitutions

### Voltage Regulators (instead of AMS1117):
- **HT7333-A**: 250mA, 0.09V dropout, ~4µA quiescent ✓ Best
- **MCP1700**: 250mA, 0.178V dropout, 1.6µA quiescent ✓ Great
- **AP2112K**: 600mA, 0.055V dropout, 55µA quiescent ✓ Good
- **XC6206**: 200mA, 0.1V dropout, 1µA quiescent ✓ Good

### Temperature/Humidity Sensors (instead of AHT10):
- **AHT20**: Improved version, same pinout
- **SHT31**: Higher accuracy, I2C, more expensive
- **DHT22**: Cheaper but higher power, slower, less accurate
- **BME280**: Adds pressure, good option if needed

### ESP Modules (instead of ESP8266):
- **ESP32-C3**: More efficient, better ADC, more memory
- **ESP32-S2**: Native USB, better for debugging
- **ESP8266-12F**: Bare module version (smaller footprint)

---

**Prototyping Tip**: Use a breadboard first, then move to perfboard or custom PCB after testing!
