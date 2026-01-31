# ESP8266 Pin Mapping Reference

## Wemos D1 Mini Pin Labels

| Wemos D1 | GPIO | Function in Project          | Notes                        |
|----------|------|------------------------------|------------------------------|
| D0       | 16   | Wake Pin (to RST via 220Ω)   | Timer wake from deep sleep   |
| D1       | 5    | I2C SCL (AHT10)              | Internal pull-up available   |
| D2       | 4    | I2C SDA (AHT10)              | Internal pull-up available   |
| D3       | 0    | Boot/Flash Button            | Config mode trigger          |
| D4       | 2    | Built-in LED                 | Status indicator (active LOW)|
| D5       | 14   | Not used                     | Available for expansion      |
| D6       | 12   | AHT10 Power Control          | Switchable sensor power      |
| D7       | 13   | Not used                     | Available for expansion      |
| D8       | 15   | Not used                     | Available for expansion      |
| A0       | ADC  | Battery Voltage Monitor      | Built-in divider on board    |
| RST      | RST  | Reset (button + D0)          | External button + timer wake |
| 3V3      | -    | Power Output (from MCP1700)  | 3.3V regulated               |
| GND      | -    | Ground                       | Common ground                |

## Critical Connections

### 1. Deep Sleep Wake Circuit
```
D0 (GPIO16) ───[220Ω]─┬─── RST
                      │
                   [10kΩ]
                      │
                     3.3V
```
The 220Ω resistor limits current during wake pulse. The 10kΩ pull-up ensures stable reset.
**Note**: This connection is required for timer-based wake from deep sleep.

### 2. Battery Voltage Monitoring
```
Battery+ ────────── Wemos A0 (built-in divider)
```
**Wemos D1 Mini has built-in voltage divider!**
- Internal divider: 220kΩ / 100kΩ
- Can measure 0V to ~4.2V safely
- No external components needed
- ADC reads 0-1023 for approximately 0-4.2V range

**Calibration recommended**:
Test with known battery voltages and adjust the calibration factor in code.

### 3. I2C Connections (AHT10)
```
3.3V ─────────────── D6 (GPIO12) ─── AHT10 VCC (switchable power)

D2 (GPIO4) ───────── AHT10 SDA (Wemos has internal pull-up)
D1 (GPIO5) ───────── AHT10 SCL (Wemos has internal pull-up)

GND ─────────────────AHT10 GND
```
**Note**: Wemos D1 Mini typically has built-in I2C pull-ups. External 4.7kΩ resistors are optional but recommended for reliability.

## Power Architecture

```
┌─────────────┐
│ 18650 Cell  │
│ 2.7V - 4.2V │
└──────┬──────┘
       │
       ├────────────────── Wemos A0 (built-in divider)
       │
       ▼
┌─────────────┐
│  MCP1700    │ Low dropout LDO
│  3.3V       │ ~178mV dropout, 1.6µA quiescent
└──────┬──────┘
       │
       ├────── Wemos 5V pin (bypasses onboard regulator)
       │       OR connect to 3.3V pin directly
       │
       └────── (D6/GPIO12 switches AHT10 power)

Key advantages of MCP1700:
- Works down to 3.5V battery (vs 4.5V for AMS1117)
- Only 1.6µA quiescent (vs 5mA for AMS1117)
- Perfect for 18650 voltage range
```

**Important**: When powering Wemos D1 Mini from external regulator, you can:
1. Feed 3.3V to the **3.3V pin** (bypasses onboard regulator completely)
2. Or feed to **5V pin** if voltage is sufficient (NOT recommended with MCP1700 at 3.3V output)

**Best practice**: Connect MCP1700 output (3.3V) directly to Wemos **3V3 pin**.

## GPIO States During Operation

| Mode          | D6/GPIO12 (AHT) | D2/GPIO4 (SDA) | D1/GPIO5 (SCL) | D4/GPIO2 (LED) | D0/GPIO16    |
|---------------|-----------------|----------------|----------------|----------------|--------------|
| Deep Sleep    | LOW (off)       | -              | -              | HIGH (off)     | Pulses RST   |
| Measuring     | HIGH (on)       | I2C Data       | I2C Clock      | LOW (on)       | LOW          |
| WiFi Upload   | LOW             | -              | -              | Blinking       | LOW          |
| Config Mode   | LOW             | -              | -              | LOW (on)       | LOW          |

**LED Behavior**:
- Measurement: ON during sensor reading
- WiFi upload: Blinking every 0.5 seconds
- Config mode: Solid ON
- Sleep: OFF

## ESP8266 Boot Mode Selection

**GPIO0 (D3) during boot determines mode:**
- HIGH (button NOT pressed): Normal boot → Run program
- LOW (button pressed): Flash mode → Programming

**GPIO15 must be LOW, GPIO2 must be HIGH** during boot:
- Most dev boards handle this automatically
- If using bare ESP-12, ensure proper pull-ups/downs

## Current Draw by State (Wemos D1 Mini + MCP1700)

| State              | Current  | Duration         | Notes                           |
|--------------------|----------|------------------|---------------------------------|
| Deep Sleep         | ~50µA    | Most of time     | Wemos has some leakage          |
| Wake & Measure     | ~80mA    | 3-5 seconds      | Includes sensor + ESP           |
| WiFi Connect       | ~120mA   | 5-10 seconds     | Peak ~170mA                     |
| WiFi Transmit      | ~100mA   | 2-5 seconds      | Depends on data size            |
| Config Mode (AP)   | ~80mA    | User-dependent   | Continuous until configured     |

**Wemos D1 Mini Deep Sleep Reality**:
- Ideal ESP8266: ~20µA
- MCP1700 quiescent: ~1.6µA
- Wemos board overhead: ~30µA (USB chip, LEDs, etc.)
- **Total typical: 50-80µA** (varies by board revision)

Some Wemos boards have higher sleep current due to:
- CH340 USB chip not fully disabled
- Power LED always on
- Voltage regulator (if not bypassed)

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
