# bottle_to_filament
System for converting PET bottles into 3D Printer filament. Uses pulltrusion.

The PCB is powered by Arduino Nano and handles: 

- Hotend temperature input (NTC100K) tuned at 230 degrees Celsius (4.7k Ohm series resistor)
- 4 MOSFETs for: heating control, hotend fan control, filament fan control, motor control (one direction)
- Filament diameter sensor analog input

Additionally the I2C bus is exposed for for convenience.

The user interface uses three components:

- 128x32 3.3v OLED display
- rotary encoder with a button
- RGB LED

Should be sufficient for controlling the finished device.

## Compatibility
Most footprints are THT or 0603 SMD, mixed. Should be less of a problem to assemble whether you just have a starter kit or have advanced components.

## Components
| Quantity | Component |
| --- | --- |
| 1 | Arduino Nano |
| 1 | Power Supply USB C: CH224K Module set to 12V PD |
| 4 | IRLZ44N |
| 3 | 100uF Electrolythic Capacitor: fits two sizes |
| 5 | 0.1uF THT 104 or SMD 0603 |
| 4 | 1N4007 THT |
| 1 | LED BAGR 5mm |
| 1 | 4,7k Ohm THT or SMD 0603 |
| 6 | 10k Ohm THT or SMD 0603 |
| 3 | 220 Ohm THT or SMD 0603 |
| 1 | Rotary Encoder Switch EC11 |
| 1 | Oled 128x32 Display |
| n | 2.54mm Sockets and Bent/Straight Headers |

# Pictures
<img width="1299" height="847" alt="image" src="https://github.com/user-attachments/assets/fed2809a-b0f6-4b99-8088-cec35e6de857" />
<img width="839" height="711" alt="image" src="https://github.com/user-attachments/assets/c3cfdd30-9b95-49a2-999d-cbe0616a674a" />
<img width="783" height="603" alt="image" src="https://github.com/user-attachments/assets/ee15162d-f530-4b5d-909d-267dffda0f6e" />

