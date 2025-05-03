# **Space Invaders Game on STM32 with SSD1306 OLED and Buzzer**


## **Description**
This project implements the classic **Space Invaders** game on an **STM32 microcontroller**, featuring:
- **SSD1306 OLED** (I2C) for graphics display
- **Buzzer** (PWM via TIM2) for sound effects
- **Push buttons** for player controls

## **Features**
✅ **Full Gameplay**:
- Left/right movement and shooting
- Lives system and scoring
- Progressive difficulty with levels

✅ **Sound Effects**:
- Shooting, explosion, and enemy movement sounds
- Game over and level-up music

✅ **User Interface**:
- Main menu and game over screens
- Score and level display

## **Hardware Requirements**
| Component | Details |
|-----------|---------|
| **Microcontroller** | STM32F103C8T6 (Blue Pill) or compatible |
| **Display** | SSD1306 OLED (128x64, I2C) |
| **Buzzer** | Passive buzzer (connected to TIM2 Channel 1 - PA0) |
| **Buttons** | 3 buttons (Left/Right/Fire) |
| **Programmer** | ST-Link or USB-to-Serial adapter |

## **Setup**
### **1. Clone the Project**
```bash
git clone https://github.com/yourusername/stm32-space-invaders.git
cd stm32-space-invaders
```

### **2. Open Project in STM32CubeIDE**
- Import the project into **STM32CubeIDE**
- Connect hardware according to the circuit diagram

### **3. Hardware Configuration**
- **OLED**: Connect SDA (PB7), SCL (PB6), VCC (3.3V), GND
- **Buzzer**: PA0 (TIM2_CH1)
- **Buttons**:
  - Left: PB5
  - Right: PB4
  - Fire: PB3

### **4. Build and Flash**
- Click **Build** (🔨) in STM32CubeIDE
- Flash the firmware using ST-Link


## **Code Structure**
```
├── Core/
│   ├── Src/main.c             # Main game loop
│   ├── Src/ssd1306.c          # OLED driver
│   └── Src/stm32f1xx_it.c     # Interrupts
├── Drivers/
├── STM32CubeIDE/              # Configuration files
└── README.md
```



## **License**
This project is licensed under the **MIT License**. See [LICENSE](LICENSE) file for details.

---

## **Support**
If you encounter issues or have improvements, please open an **issue** or **pull request**!  
🌟 **Don't forget to star the repo if you find it useful!** 🌟
