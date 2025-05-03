/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    int16_t X;
    int16_t Y;
    uint8_t Status;  // 0 active, 1 exploding, 2 destroyed
} GameObjectStruct;

typedef struct {
    GameObjectStruct Ord;
    uint8_t ExplosionGfxCounter;
} AlienStruct;

typedef struct {
    GameObjectStruct Ord;
    uint16_t Score;
    uint8_t Lives;
    uint8_t Level;
    uint8_t AliensDestroyed;    // count of how many killed so far
    uint8_t AlienSpeed;         // higher the number slower they go, calculated when ever alien destroyed
    uint8_t ExplosionGfxCounter; // how long we want the ExplosionGfx to last
} PlayerStruct;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCREEN_WIDTH SSD1306_WIDTH
#define SCREEN_HEIGHT SSD1306_HEIGHT
// Input settings - adjust these to your STM32 GPIO pins
#define FIRE_BUT_PIN fireButton_Pin
#define FIRE_BUT_PORT fireButton_GPIO_Port
#define RIGHT_BUT_PIN rightButton_Pin
#define RIGHT_BUT_PORT rightButton_GPIO_Port
#define LEFT_BUT_PIN leftButton_Pin
#define LEFT_BUT_PORT leftButton_GPIO_Port

#define BUZZER_PIN GPIO_PIN_0
#define BUZZER_PORT GPIOA
#define BUZZER_TIMER &htim2  // Use a timer for PWM control
// Alien settings
#define ALIEN_HEIGHT 8
#define NUM_ALIEN_COLUMNS 7
#define NUM_ALIEN_ROWS 3
#define X_START_OFFSET 6
#define SPACE_BETWEEN_ALIEN_COLUMNS 5
#define LARGEST_ALIEN_WIDTH 11
#define SPACE_BETWEEN_ROWS 9
#define INVADERS_DROP_BY 4
#define INVADERS_SPEED 12
#define INVADER_HEIGHT 8
#define EXPLOSION_GFX_TIME 7

//Mothership settings
#define MOTHERSHIP_HEIGHT 4
#define MOTHERSHIP_WIDTH 16
#define MOTHERSHIP_SPEED 2
#define MOTHERSHIP_SPAWN_CHANCE 250         //HIGHER IS LESS CHANCE OF SPAWN
#define DISPLAY_MOTHERSHIP_BONUS_TIME 20    // how long bonus stays on screen for displaying mothership

#define INVADERS_Y_START MOTHERSHIP_HEIGHT-1
#define AMOUNT_TO_DROP_BY_PER_LEVEL 4 // How much farther down aliens start per new level
#define LEVEL_TO_RESET_TO_START_HEIGHT 4  // EVERY MULTIPLE OF THIS LEVEL THE ALIEN y START POSITION WILL RESET TO TOP

#define ALIEN_X_MOVE_AMOUNT 1         // number of pixels moved at start of wave
#define CHANCEOFBOMBDROPPING 20       // Higher the number the rarer the bomb drop,
#define BOMB_HEIGHT 4
#define BOMB_WIDTH 2
#define MAXBOMBS 3

// Player settings
#define TANKGFX_WIDTH 13
#define TANKGFX_HEIGHT 8
#define PLAYER_X_MOVE_AMOUNT 2
#define LIVES 3
#define PLAYER_EXPLOSION_TIME 10      // How long an ExplosionGfx remains on screen before dissapearing
#define PLAYER_Y_START 56
#define PLAYER_X_START 0

// Missile settings
#define MISSILE_HEIGHT 4
#define MISSILE_WIDTH 1
#define MISSILE_SPEED 4

// Status constants
#define ACTIVE 0
#define EXPLODING 1
#define DESTROYED 2

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
// Game objects
AlienStruct Alien[NUM_ALIEN_COLUMNS][NUM_ALIEN_ROWS];
AlienStruct MotherShip;
GameObjectStruct AlienBomb[MAXBOMBS];

static const int TOTAL_ALIENS = NUM_ALIEN_COLUMNS * NUM_ALIEN_ROWS;

// Alien properties
uint8_t AlienWidth[] = {8, 11, 12};  // top, middle, bottom widths
int8_t AlienXMoveAmount = 2;
int8_t InvadersMoveCounter;            // counts down, when 0 move invaders, set according to how many aliens on screen
uint8_t AnimationFrame = 0;   // two frames of animation, if true show one if false show the other

// Mothership
int8_t MotherShipSpeed;
uint16_t MotherShipBonus;
int16_t MotherShipBonusXPos;            // pos to display bonus at
uint8_t MotherShipBonusCounter;         // how long bonus amount left on screen

// Player global variables
PlayerStruct Player;
GameObjectStruct Missile;
// game variables
uint16_t HiScore = 0;
uint8_t GameInPlay = 0;

// Global/static UFO variables
static int ufoX = -20;
static uint32_t lastUfoMove = 0;
static uint32_t lastUfoStart = 0;
static uint8_t ufoActive = 0;

// Graphics
const uint8_t InvaderTopGfx[] = {
  0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x5A, 0xA5
};

const uint8_t InvaderTopGfx2[] = {
  0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x5A, 0x81, 0x42
};

const uint8_t InvaderMiddleGfx[] = {
  0x20, 0x80, 0x11, 0x00, 0x3F, 0x80, 0x6E, 0xC0,
  0xFF, 0xE0, 0xBF, 0xA0, 0xA0, 0xA0, 0x1B, 0x00
};

const uint8_t InvaderMiddleGfx2[] = {
  0x20, 0x80, 0x10, 0x10, 0xBF, 0xA0, 0xAE, 0xA0,
  0xFF, 0xE0, 0x3F, 0x80, 0x20, 0x80, 0x40, 0x40
};

const uint8_t InvaderBottomGfx[] = {
  0x0F, 0x00, 0x7F, 0xE0, 0xFF, 0xF0, 0xE6, 0x70,
  0xFF, 0xF0, 0x39, 0xC0, 0x66, 0x60, 0x30, 0xC0
};

const uint8_t InvaderBottomGfx2[] = {
  0x0F, 0x00, 0x7F, 0xE0, 0xFF, 0xF0, 0xE6, 0x70,
  0xFF, 0xF0, 0x39, 0xC0, 0x46, 0x20, 0x80, 0x10
};

const uint8_t MotherShipGfx [] = {
  0x3F, 0xFC,
  0x6D, 0xB6,
  0xFF, 0xFF,
  0x39, 0x9C
};

// Player graphics
const uint8_t TankGfx[] ={
	0x02, 0x00, 0x27, 0x20, 0x2f, 0xa0, 0x3f, 0xe0,
	0x7f, 0xf0, 0xff, 0xf8, 0x6d, 0xb0, 0x25, 0x20
};

const uint8_t ExplosionGfx[] = {
  0x08, 0x80, 0x45, 0x10, 0x20, 0x20, 0x10, 0x40,
  0xC0, 0x18, 0x10, 0x40, 0x25, 0x20, 0x48, 0x90
};
// Missile graphics
const uint8_t MissileGfx[] = {
  0x80, 0x80, 0x80, 0x80
};

static const uint8_t AlienBombGfx [] = {
  0x80,
  0x40,
  0x80,
  0x40
};

// Alien sprites (2 frames 8x8)
const uint8_t alien1[8] = {
    0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x42, 0x81
};

const uint8_t alien2[8] = {
    0x18, 0x3C, 0x7E, 0xBD, 0xFF, 0x24, 0x5A, 0xA5
};

// UFO sprite (16x8)
const uint8_t ufoSprite[8][2] = {
    {0x0E, 0x00}, {0x1F, 0x00}, {0x3F, 0x80}, {0x7F, 0xC0},
    {0xFF, 0xE0}, {0x3F, 0x80}, {0x1F, 0x00}, {0x0E, 0x00}
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void InitAliens(int YStart);
void InitPlayer(void);
void AttractScreen(void);
void Physics(void);
uint8_t GetScoreForAlien(int RowNumber);
void MotherShipPhysics(void);
void PlayerControl(void);
void MissileControl(void);
void AlienControl(void);
void MoveBombs(void);
void DropBomb(void);
void BombCollisions(void);
void PlayerHit(void);
void CheckCollisions(void);
void MotherShipCollisions(void);
void MissileAndAlienCollisions(void);
uint8_t Collision(GameObjectStruct Obj1, uint8_t Width1, uint8_t Height1,
                 GameObjectStruct Obj2, uint8_t Width2, uint8_t Height2);
int16_t RightMostPos(void);
int16_t LeftMostPos(void);
uint8_t ReadButton(GPIO_TypeDef* port, uint16_t pin);
void UpdateDisplay(void);
void LoseLife(void);
void GameOver(void);
void DisplayPlayerAndLives(PlayerStruct *Player);
void CentreText(const char *Text, uint8_t Y);
void NextLevel(PlayerStruct *Player);
void NewGame(void);
uint8_t DebouncedReadButton(GPIO_TypeDef* port, uint16_t pin) {
    if(ReadButton(port, pin) == GPIO_PIN_RESET) {
        HAL_Delay(20);  // Chờ 20ms để chống nhiễu
        if(ReadButton(port, pin) == GPIO_PIN_RESET) {
            return 1;
        }
    }
    return 0;
}
uint8_t ReadButton(GPIO_TypeDef* port, uint16_t pin);
// Random number generation
int my_random(int max) {
    return rand() % max;
}

void randomSeed(uint32_t seed) {
    srand(seed);
}

void DrawAlien(uint8_t x, uint8_t y, const uint8_t* sprite) {
    for (uint8_t row = 0; row < 8; row++) {
        uint8_t rowBits = sprite[row];
        for (uint8_t col = 0; col < 8; col++) {
            if (rowBits & (1 << (7 - col))) {
                ssd1306_DrawPixel(x + col, y + row, White);
            }
        }
    }
}

void DrawUFO(uint8_t x, uint8_t y) {
    for (uint8_t row = 0; row < 8; row++) {
        uint16_t rowBits = (ufoSprite[row][0] << 8) | ufoSprite[row][1];
        for (uint8_t col = 0; col < 16; col++) {
            if (rowBits & (1 << (15 - col))) {
                ssd1306_DrawPixel(x + col, y + row, White);
            }
        }
    }
}
// Sound effect functions
void PlayTone(uint32_t frequency, uint32_t duration);
void PlayShootSound(void);
void PlayExplosionSound(void);
void PlayInvaderMoveSound(void);
void PlayMothershipSound(void);
void PlayGameOverSound(void);
void PlayLevelUpSound(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  ssd1306_Fill(Black);

  InitAliens(0);
  InitPlayer();

  // Initialize buttons - should be configured in CubeMX
  // GPIO_InitTypeDef GPIO_InitStruct = {0};
  // Configure your button GPIOs here

  // Initialize random number generator
  randomSeed(HAL_GetTick());

  // Load high score from flash (simplified)
  HiScore = 0; // In a real implementation, you'd read from flash
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    if(GameInPlay) {
	        Physics();
	        UpdateDisplay();
	    } else {
	        AttractScreen();
	    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_DISABLE_OCxPRELOAD(&htim2, TIM_CHANNEL_1);
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : rightButton_Pin */
  GPIO_InitStruct.Pin = rightButton_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(rightButton_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : fireButton_Pin leftButton_Pin */
  GPIO_InitStruct.Pin = fireButton_Pin|leftButton_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Game_Init(void) {
    // Initialize OLED display
    ssd1306_Init();
    ssd1306_Fill(Black);

    InitAliens(0);
    InitPlayer();

    // Initialize buttons - should be configured in CubeMX
    // GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Configure your button GPIOs here

    // Initialize random number generator
    randomSeed(HAL_GetTick());

    // Load high score from flash (simplified)
    HiScore = 0; // In a real implementation, you'd read from flash
}

void AttractScreen(void) {
    ssd1306_Fill(Black);

    CentreText("Space Invaders", 0);
    CentreText("Play", 12);

    // Blinking "Press A to start"
    static uint32_t lastBlink = 0;
    static uint8_t blinkState = 0;
    if (HAL_GetTick() - lastBlink > 500) {
        blinkState = !blinkState;
        lastBlink = HAL_GetTick();
    }

    if (blinkState) {
        CentreText("Press A to start", 24);
    }

    // Hi Score
    CentreText("Hi Score     ", 36);
    char scoreStr[10];
    sprintf(scoreStr, "%d", HiScore);
    ssd1306_SetCursor(80, 36);
    ssd1306_WriteString(scoreStr, Font_7x10, White);

    CentreText("< Move >     A: Fire", 46);

    // Alien animation
    const uint8_t* alienSprite = blinkState ? alien1 : alien2;
    DrawAlien(32, 56, alienSprite);
    const uint8_t* alienSprite2 = blinkState ? InvaderTopGfx : InvaderTopGfx2;
    DrawAlien(48, 56, alienSprite2);
    DrawAlien(64, 56, alienSprite);
    DrawAlien(80, 56, alienSprite2);

    // UFO animation
    uint32_t now = HAL_GetTick();
    if (!ufoActive && now - lastUfoStart > 5000) {
        ufoActive = 1;
        ufoX = -20;
        lastUfoMove = now;
        lastUfoStart = now;

        // Optional: UFO sound
        // PlayTone(1000, 200);
    }

    if (ufoActive && now - lastUfoMove > 50) {
        ufoX++;
        lastUfoMove = now;

        if (ufoX > 128) {
            ufoActive = 0;
        }
    }

    if (ufoActive) {
        DrawUFO(ufoX, 0);  // Bay ở hàng trên cùng
    }
    ssd1306_UpdateScreen();

    if(ReadButton(fireButton_GPIO_Port, fireButton_Pin) == GPIO_PIN_RESET) {
        GameInPlay = 1;
        NewGame();
    }

//    // CHECK FOR HIGH SCORE RESET, PLAYER MUST HOLD LEFT AND RIGHT TOGETHER
//    if((digitalRead(LEFT_BUT_PORT, LEFT_BUT_PIN) == 1) && (digitalRead(RIGHT_BUT_PORT, RIGHT_BUT_PIN) == 1)) {
//        HiScore = 0;
//        // In a real implementation, you'd write to flash here
//    }
}

void Game_Loop(void) {
    if(GameInPlay) {
        Physics();
        UpdateDisplay();
    } else {
        AttractScreen();
    }
}

void Physics(void) {
    if(Player.Ord.Status == ACTIVE) {
        AlienControl();
        MotherShipPhysics();
        PlayerControl();
        MissileControl();
        CheckCollisions();
    }
}

uint8_t GetScoreForAlien(int RowNumber) {
    switch (RowNumber) {
        case 0: return 30;
        case 1: return 20;
        case 2: return 10;
        default: return 0;
    }
}

void MotherShipPhysics(void) {
    if(MotherShip.Ord.Status == ACTIVE) {
        MotherShip.Ord.X += MotherShipSpeed;
        if(MotherShipSpeed > 0) {
            if(MotherShip.Ord.X >= SCREEN_WIDTH) {
                MotherShip.Ord.Status = DESTROYED;
            }
        } else {
            if(MotherShip.Ord.X + MOTHERSHIP_WIDTH < 0) {
                MotherShip.Ord.Status = DESTROYED;
            }
        }
    } else {
        if(my_random(MOTHERSHIP_SPAWN_CHANCE) == 1) {
            MotherShip.Ord.Status = ACTIVE;
            if(my_random(2) == 1) {
                MotherShip.Ord.X = SCREEN_WIDTH;
                MotherShipSpeed = -MOTHERSHIP_SPEED;
            } else {
                MotherShip.Ord.X = -MOTHERSHIP_WIDTH;
                MotherShipSpeed = MOTHERSHIP_SPEED;
            }
        }
    }
}

void PlayerControl(void) {
    uint8_t firePressed = DebouncedReadButton(FIRE_BUT_PORT, FIRE_BUT_PIN);

    if((ReadButton(RIGHT_BUT_PORT, RIGHT_BUT_PIN) == 1) && (Player.Ord.X + TANKGFX_WIDTH < SCREEN_WIDTH)) {
        Player.Ord.X += PLAYER_X_MOVE_AMOUNT;
    }
    if((ReadButton(LEFT_BUT_PORT, LEFT_BUT_PIN) == 1) && (Player.Ord.X > 0)) {
        Player.Ord.X -= PLAYER_X_MOVE_AMOUNT;
    }
    if(firePressed && (Missile.Status != ACTIVE)) {
        Missile.X = Player.Ord.X + (TANKGFX_WIDTH / 2);
        Missile.Y = PLAYER_Y_START;
        Missile.Status = ACTIVE;
        PlayShootSound();
    }
}

void MissileControl(void) {
    if(Missile.Status == ACTIVE) {
        Missile.Y -= MISSILE_SPEED;
        if(Missile.Y + MISSILE_HEIGHT < 0) {
            Missile.Status = DESTROYED;
        }
    }
}

void AlienControl(void) {
    if((InvadersMoveCounter--) < 0) {
    	PlayInvaderMoveSound();
        uint8_t Dropped = 0;
        if((RightMostPos() + AlienXMoveAmount >= SCREEN_WIDTH) || (LeftMostPos() + AlienXMoveAmount < 0)) {
            AlienXMoveAmount = -AlienXMoveAmount;
            Dropped = 1;
        }

        for(int Across = 0; Across < NUM_ALIEN_COLUMNS; Across++) {
            for(int Down = 0; Down < 3; Down++) {
                if(Alien[Across][Down].Ord.Status == ACTIVE) {
                    if(Dropped == 0) {
                        Alien[Across][Down].Ord.X += AlienXMoveAmount;
                    } else {
                        Alien[Across][Down].Ord.Y += INVADERS_DROP_BY;
                    }
                }
            }
        }

        InvadersMoveCounter = Player.AlienSpeed;
        AnimationFrame = !AnimationFrame;
    }

    if(my_random(CHANCEOFBOMBDROPPING) == 1) {
        DropBomb();
    }
    MoveBombs();
}

void MoveBombs(void) {
    for(int i = 0; i < MAXBOMBS; i++) {
        if(AlienBomb[i].Status == ACTIVE) {
            AlienBomb[i].Y += 2;
        }
    }
}

void DropBomb(void) {
    uint8_t Free = 0;
    uint8_t ActiveCols[NUM_ALIEN_COLUMNS];
    uint8_t BombIdx = 0;

    while((Free == 0) && (BombIdx < MAXBOMBS)) {
        if(AlienBomb[BombIdx].Status == DESTROYED) {
            Free = 1;
        } else {
            BombIdx++;
        }
    }

    if(Free) {
        uint8_t Columns = 0;
        uint8_t ActiveColCount = 0;
        int8_t Row;
        uint8_t ChosenColumn;

        while(Columns < NUM_ALIEN_COLUMNS) {
            Row = 2;
            while(Row >= 0) {
                if(Alien[Columns][Row].Ord.Status == ACTIVE) {
                    ActiveCols[ActiveColCount] = Columns;
                    ActiveColCount++;
                    break;
                }
                Row--;
            }
            Columns++;
        }

        if(ActiveColCount > 0) {
            ChosenColumn = my_random(ActiveColCount);
            Row = 2;
            while(Row >= 0) {
                if(Alien[ActiveCols[ChosenColumn]][Row].Ord.Status == ACTIVE) {
                    AlienBomb[BombIdx].Status = ACTIVE;
                    AlienBomb[BombIdx].X = Alien[ActiveCols[ChosenColumn]][Row].Ord.X + (AlienWidth[Row] / 2);
                    AlienBomb[BombIdx].X = (AlienBomb[BombIdx].X - 2) + my_random(4);
                    AlienBomb[BombIdx].Y = Alien[ActiveCols[ChosenColumn]][Row].Ord.Y + 4;
                    break;
                }
                Row--;
            }
        }
    }
}

void BombCollisions(void) {
    for(int i = 0; i < MAXBOMBS; i++) {
        if(AlienBomb[i].Status == ACTIVE) {
            if(AlienBomb[i].Y > 64) {
                AlienBomb[i].Status = DESTROYED;
            } else {
                if(Collision(AlienBomb[i], BOMB_WIDTH, BOMB_HEIGHT, Missile, MISSILE_WIDTH, MISSILE_HEIGHT)) {
                    AlienBomb[i].Status = EXPLODING;
                    Missile.Status = DESTROYED;
                } else {
                    if(Collision(AlienBomb[i], BOMB_WIDTH, BOMB_HEIGHT, Player.Ord, TANKGFX_WIDTH, TANKGFX_HEIGHT)) {
                        PlayerHit();
                        AlienBomb[i].Status = DESTROYED;
                    }
                }
            }
        }
    }
}

void PlayerHit(void) {
    Player.Ord.Status = EXPLODING;
    Player.ExplosionGfxCounter = PLAYER_EXPLOSION_TIME;
    Missile.Status = DESTROYED;
    PlayExplosionSound();
}

void CheckCollisions(void) {
    MissileAndAlienCollisions();
    MotherShipCollisions();
    BombCollisions();
}

void MotherShipCollisions(void) {
    if((Missile.Status == ACTIVE) && (MotherShip.Ord.Status == ACTIVE)) {
        if(Collision(Missile, MISSILE_WIDTH, MISSILE_HEIGHT, MotherShip.Ord, MOTHERSHIP_WIDTH, MOTHERSHIP_HEIGHT)) {
            MotherShip.Ord.Status = EXPLODING;
            MotherShip.ExplosionGfxCounter = EXPLOSION_GFX_TIME;
            Missile.Status = DESTROYED;

            MotherShipBonus = my_random(4);
            switch(MotherShipBonus) {
                case 0: MotherShipBonus = 50; break;
                case 1: MotherShipBonus = 100; break;
                case 2: MotherShipBonus = 150; break;
                case 3: MotherShipBonus = 300; break;
            }

            Player.Score += MotherShipBonus;
            MotherShipBonusXPos = MotherShip.Ord.X;

            if(MotherShipBonusXPos > 100) {
                MotherShipBonusXPos = 100;
            }
            if(MotherShipBonusXPos < 0) {
                MotherShipBonusXPos = 0;
            }

            MotherShipBonusCounter = DISPLAY_MOTHERSHIP_BONUS_TIME;
        }
    }
}

void MissileAndAlienCollisions(void) {
    for(int across = 0; across < NUM_ALIEN_COLUMNS; across++) {
        for(int down = 0; down < NUM_ALIEN_ROWS; down++) {
            if(Alien[across][down].Ord.Status == ACTIVE) {
                if(Missile.Status == ACTIVE) {
                    if(Collision(Missile, MISSILE_WIDTH, MISSILE_HEIGHT, Alien[across][down].Ord, AlienWidth[down], INVADER_HEIGHT)) {
                        Alien[across][down].Ord.Status = EXPLODING;
                        Missile.Status = DESTROYED;
                        Player.Score += GetScoreForAlien(down);
                        Player.AliensDestroyed++;

                        Player.AlienSpeed = ((1 - (Player.AliensDestroyed / (float)TOTAL_ALIENS)) * INVADERS_SPEED);

                        if(Player.AliensDestroyed == TOTAL_ALIENS - 2) {
                            if(AlienXMoveAmount > 0) {
                                AlienXMoveAmount = ALIEN_X_MOVE_AMOUNT * 2;
                            } else {
                                AlienXMoveAmount = -(ALIEN_X_MOVE_AMOUNT * 2);
                            }
                        }

                        if(Player.AliensDestroyed == TOTAL_ALIENS - 1) {
                            if(AlienXMoveAmount > 0) {
                                AlienXMoveAmount = ALIEN_X_MOVE_AMOUNT * 4;
                            } else {
                                AlienXMoveAmount = -(ALIEN_X_MOVE_AMOUNT * 4);
                            }
                        }

                        if(Player.AliensDestroyed == TOTAL_ALIENS) {
                            NextLevel(&Player);
                        }
                    }
                }

                if(Alien[across][down].Ord.Status == ACTIVE) {
                    if(Collision(Player.Ord, TANKGFX_WIDTH, TANKGFX_HEIGHT, Alien[across][down].Ord, AlienWidth[down], ALIEN_HEIGHT)) {
                        PlayerHit();
                    } else {
                        if(Alien[across][down].Ord.Y + 8 > SCREEN_HEIGHT) {
                            PlayerHit();
                        }
                    }
                }
            }
        }
    }
}

uint8_t Collision(GameObjectStruct Obj1, uint8_t Width1, uint8_t Height1, GameObjectStruct Obj2, uint8_t Width2, uint8_t Height2) {
    return ((Obj1.X + Width1 > Obj2.X) && (Obj1.X < Obj2.X + Width2) &&
           (Obj1.Y + Height1 > Obj2.Y) && (Obj1.Y < Obj2.Y + Height2));
}

int16_t RightMostPos(void) {
    int16_t Across = NUM_ALIEN_COLUMNS - 1;
    int16_t Down;
    int16_t Largest = 0;
    int16_t RightPos;

    while(Across >= 0) {
        Down = 0;
        while(Down < NUM_ALIEN_ROWS) {
            if(Alien[Across][Down].Ord.Status == ACTIVE) {
                RightPos = Alien[Across][Down].Ord.X + AlienWidth[Down];
                if(RightPos > Largest) {
                    Largest = RightPos;
                }
            }
            Down++;
        }

        if(Largest > 0) {
            return Largest;
        }
        Across--;
    }
    return 0;
}

int16_t LeftMostPos(void) {
    int16_t Across = 0;
    int16_t Down;
    int16_t Smallest = SCREEN_WIDTH * 2;

    while(Across < NUM_ALIEN_COLUMNS) {
        Down = 0;
        while(Down < 3) {
            if(Alien[Across][Down].Ord.Status == ACTIVE) {
                if(Alien[Across][Down].Ord.X < Smallest) {
                    Smallest = Alien[Across][Down].Ord.X;
                }
            }
            Down++;
        }

        if(Smallest < SCREEN_WIDTH * 2) {
            return Smallest;
        }
        Across++;
    }
    return 0;
}

void UpdateDisplay(void) {
    ssd1306_Fill(Black);

    // Mothership bonus display if required
    if(MotherShipBonusCounter > 0) {
        char bonusStr[10];
        sprintf(bonusStr, "%d", MotherShipBonus);
        ssd1306_SetCursor(MotherShipBonusXPos, 0);
        ssd1306_WriteString(bonusStr, Font_7x10, 1);
        MotherShipBonusCounter--;
    } else {
        // draw score and lives
        char scoreStr[10];
        sprintf(scoreStr, "%d", Player.Score);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString(scoreStr, Font_7x10, 1);

        char livesStr[5];
        sprintf(livesStr, "%d", Player.Lives);
        ssd1306_SetCursor(SCREEN_WIDTH - 7, 0);
        ssd1306_WriteString(livesStr, Font_7x10, 1);
    }

    // BOMBS
    for(int i = 0; i < MAXBOMBS; i++) {
        if(AlienBomb[i].Status == ACTIVE) {
            ssd1306_DrawBitmap(AlienBomb[i].X, AlienBomb[i].Y, AlienBombGfx, BOMB_WIDTH, BOMB_HEIGHT, White);
        } else if(AlienBomb[i].Status == EXPLODING) {
        	ssd1306_DrawBitmap(AlienBomb[i].X - 4, AlienBomb[i].Y, ExplosionGfx, 8, 8, 1);
            AlienBomb[i].Status = DESTROYED;
        }
    }

    // Invaders
    for(int across = 0; across < NUM_ALIEN_COLUMNS; across++) {
        for(int down = 0; down < NUM_ALIEN_ROWS; down++) {
            if(Alien[across][down].Ord.Status == ACTIVE) {
                switch(down) {
                    case 0:
                        if(AnimationFrame) {
                        	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                             InvaderTopGfx, AlienWidth[down], INVADER_HEIGHT, 1);
                        } else {
                        	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                             InvaderTopGfx2, AlienWidth[down], INVADER_HEIGHT, 1);
                        }
                        break;
                    case 1:
                        if(AnimationFrame) {
                        	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                             InvaderMiddleGfx, AlienWidth[down], INVADER_HEIGHT, 1);
                        } else {
                        	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                             InvaderMiddleGfx2, AlienWidth[down], INVADER_HEIGHT, 1);
                        }
                        break;
                    default:
                        if(AnimationFrame) {
                        	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                             InvaderBottomGfx, AlienWidth[down], INVADER_HEIGHT, 1);
                        } else {
                        	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                             InvaderBottomGfx2, AlienWidth[down], INVADER_HEIGHT, 1);
                        }
                }
            } else if(Alien[across][down].Ord.Status == EXPLODING) {
                Alien[across][down].ExplosionGfxCounter--;
                if(Alien[across][down].ExplosionGfxCounter > 0) {
                	ssd1306_DrawBitmap(Alien[across][down].Ord.X, Alien[across][down].Ord.Y,
                                     ExplosionGfx, 13, 8, 1);
                } else {
                    Alien[across][down].Ord.Status = DESTROYED;
                }
            }
        }
    }

    // player
    if(Player.Ord.Status == ACTIVE) {
    	ssd1306_DrawBitmap(Player.Ord.X, Player.Ord.Y, TankGfx, TANKGFX_WIDTH, TANKGFX_HEIGHT, 1);
    } else if(Player.Ord.Status == EXPLODING) {
        for(int i = 0; i < TANKGFX_WIDTH; i += 2) {
        	ssd1306_DrawBitmap(Player.Ord.X + i, Player.Ord.Y, ExplosionGfx, my_random(4) + 2, 8, 1);
        }
        Player.ExplosionGfxCounter--;
        if(Player.ExplosionGfxCounter == 0) {
            Player.Ord.Status = DESTROYED;
            LoseLife();
        }
    }

    // missile
    if(Missile.Status == ACTIVE) {
    	ssd1306_DrawBitmap(Missile.X, Missile.Y, MissileGfx, MISSILE_WIDTH, MISSILE_HEIGHT, 1);
    }

    // mothership
    if(MotherShip.Ord.Status == ACTIVE) {
    	ssd1306_DrawBitmap(MotherShip.Ord.X, MotherShip.Ord.Y, MotherShipGfx, MOTHERSHIP_WIDTH, MOTHERSHIP_HEIGHT, 1);
    } else if(MotherShip.Ord.Status == EXPLODING) {
        for(int i = 0; i < MOTHERSHIP_WIDTH; i += 2) {
        	ssd1306_DrawBitmap(MotherShip.Ord.X + i, MotherShip.Ord.Y, ExplosionGfx, my_random(4) + 2, MOTHERSHIP_HEIGHT, 1);
        }
        MotherShip.ExplosionGfxCounter--;
        if(MotherShip.ExplosionGfxCounter == 0) {
            MotherShip.Ord.Status = DESTROYED;
        }
    }

    ssd1306_UpdateScreen();
}

void LoseLife(void) {
    Player.Lives--;
    if(Player.Lives > 0) {
        DisplayPlayerAndLives(&Player);

        for(int i = 0; i < MAXBOMBS; i++) {
            AlienBomb[i].Status = DESTROYED;
            AlienBomb[i].Y = 0;
        }

        Player.Ord.Status = ACTIVE;
        Player.Ord.X = 0;
    } else {
        GameOver();
    }
}


void GameOver(void) {
    GameInPlay = 0;
    PlayGameOverSound();
    ssd1306_Fill(Black);

    CentreText("Player 1", 0);
    CentreText("Game Over", 12);
    CentreText("Score ", 24);

    char scoreStr[10];
    sprintf(scoreStr, "%d", Player.Score);
    ssd1306_WriteString(scoreStr, Font_7x10, White);

    if(Player.Score > HiScore) {
        CentreText("NEW HIGH SCORE!!!", 36);
        CentreText("**CONGRATULATIONS**", 48);
        HiScore = Player.Score;
        // In a real implementation, you'd write to flash here
    }

    ssd1306_UpdateScreen();
    HAL_Delay(2500);
}

void DisplayPlayerAndLives(PlayerStruct *Player) {
    ssd1306_Fill(Black);

    CentreText("Player 1", 0);
    CentreText("Score ", 12);

    char scoreStr[10];
    sprintf(scoreStr, "%d", Player->Score);
    ssd1306_WriteString(scoreStr, Font_7x10, White);

    CentreText("Lives ", 24);

    char livesStr[5];
    sprintf(livesStr, "%d", Player->Lives);
    ssd1306_WriteString(livesStr, Font_7x10, White);

    CentreText("Level ", 36);

    char levelStr[5];
    sprintf(levelStr, "%d", Player->Level);
    ssd1306_WriteString(levelStr, Font_7x10, White);

    ssd1306_UpdateScreen();
    HAL_Delay(2000);

    Player->Ord.X = PLAYER_X_START;
}

void CentreText(const char *Text, uint8_t Y) {
	uint8_t x = (SCREEN_WIDTH / 2) - ((strlen(Text) * 7) / 2);
    ssd1306_SetCursor(x, Y);
    ssd1306_WriteString((char*)Text, Font_7x10, White);
}

void NextLevel(PlayerStruct *Player) {
	PlayLevelUpSound();
    for(int i = 0; i < MAXBOMBS; i++) {
        AlienBomb[i].Status = DESTROYED;
    }

    AnimationFrame = 0;
    Player->Level++;

    int YStart = ((Player->Level - 1) % LEVEL_TO_RESET_TO_START_HEIGHT) * AMOUNT_TO_DROP_BY_PER_LEVEL;
    InitAliens(YStart);

    AlienXMoveAmount = ALIEN_X_MOVE_AMOUNT;
    Player->AlienSpeed = INVADERS_SPEED;
    Player->AliensDestroyed = 0;

    MotherShip.Ord.X = -MOTHERSHIP_WIDTH;
    MotherShip.Ord.Status = DESTROYED;
    Missile.Status = DESTROYED;

    randomSeed(HAL_GetTick());
    DisplayPlayerAndLives(Player);
}

void NewGame(void) {
    InitPlayer();
    NextLevel(&Player);
}

void InitPlayer(void) {
    Player.Ord.Y = PLAYER_Y_START;
    Player.Ord.X = PLAYER_X_START;
    Player.Ord.Status = ACTIVE;
    Player.Lives = LIVES;
    Player.Level = 0;
    Missile.Status = DESTROYED;
    Player.Score = 0;
}

void InitAliens(int YStart) {
    for(int across = 0; across < NUM_ALIEN_COLUMNS; across++) {
        for(int down = 0; down < 3; down++) {
            Alien[across][down].Ord.X = X_START_OFFSET + (across * (LARGEST_ALIEN_WIDTH + SPACE_BETWEEN_ALIEN_COLUMNS)) - (AlienWidth[down] / 2);
            Alien[across][down].Ord.Y = YStart + (down * SPACE_BETWEEN_ROWS);
            Alien[across][down].Ord.Status = ACTIVE;
            Alien[across][down].ExplosionGfxCounter = EXPLOSION_GFX_TIME;
        }
    }

    MotherShip.Ord.Y = 0;
    MotherShip.Ord.X = -MOTHERSHIP_WIDTH;
    MotherShip.Ord.Status = DESTROYED;
}

uint8_t ReadButton(GPIO_TypeDef* port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

/* USER CODE BEGIN 4 */
void PlayTone(uint32_t frequency, uint32_t duration) {
    if(frequency == 0) {
        HAL_TIM_PWM_Stop(BUZZER_TIMER, TIM_CHANNEL_1);
        return;
    }
    HAL_TIM_PWM_Start(BUZZER_TIMER, TIM_CHANNEL_1);
    HAL_Delay(duration);
    HAL_TIM_PWM_Stop(BUZZER_TIMER, TIM_CHANNEL_1);
}


void PlayShootSound(void) {
    PlayTone(800, 50);
}

void PlayExplosionSound(void) {
    PlayTone(200, 100);
    HAL_Delay(20);
    PlayTone(150, 100);
}

void PlayInvaderMoveSound(void) {
    static uint8_t alt = 0;
    if(alt) {
        PlayTone(440, 30);
    } else {
        PlayTone(523, 30);
    }
    alt = !alt;
}

void PlayGameOverSound(void) {
    PlayTone(300, 200);
    HAL_Delay(100);
    PlayTone(200, 300);
    HAL_Delay(100);
    PlayTone(100, 500);
}

void PlayLevelUpSound(void) {
    PlayTone(523, 100);
    HAL_Delay(50);
    PlayTone(659, 100);
    HAL_Delay(50);
    PlayTone(784, 200);
}
/* USER CODE END 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
