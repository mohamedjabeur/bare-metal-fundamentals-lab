#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

// LED Location for STM32F446RE Nucleo Board
// Port: A
// Pin: 5 (LD2 onboard LED)

// ============================================================
// DIFFERENCE 1: Bus connection
// STM32F103: GPIO on APB2 bus
// STM32F446: GPIO on AHB1 bus (faster clock = faster GPIO)
// ============================================================
#define PERIPH_BASE             (0x40000000UL)
#define AHB1PERIPH_OFFSET       (0x00020000UL)
#define AHB1PERIPH_BASE         (PERIPH_BASE + AHB1PERIPH_OFFSET) // 0x4002 0000

#define GPIOA_OFFSET            (0x00000000UL)
#define GPIOA_BASE              (AHB1PERIPH_BASE + GPIOA_OFFSET)  // 0x4002 0000

#define RCC_OFFSET              (0x00003800UL)
#define RCC_BASE                (AHB1PERIPH_BASE + RCC_OFFSET)    // 0x4002 3800

// ============================================================
// DIFFERENCE 2: Clock enable register & bit
// STM32F103: RCC_APB2ENR, bit 2 (GPIOAEN)
// STM32F446: RCC_AHB1ENR, bit 0 (GPIOAEN)
// ============================================================
#define GPIOA_EN                (1U<<0)

#define PIN5                    (1U<<5)
#define LED_PIN                 PIN5

// ============================================================
// DIFFERENCE 3: GPIO Register Structure
// STM32F103: CRL (config low) + CRH (config high) for pin mode
// STM32F446: MODER (mode register) — 2 bits per pin:
//   00 = Input, 01 = Output, 10 = Alternate Function, 11 = Analog
// ============================================================
typedef struct {
	volatile uint32_t MODER;    // 0x00 - Mode register
	volatile uint32_t OTYPER;   // 0x04 - Output type
	volatile uint32_t OSPEEDR;  // 0x08 - Output speed
	volatile uint32_t PUPDR;    // 0x0C - Pull-up/Pull-down
	volatile uint32_t IDR;      // 0x10 - Input data
	volatile uint32_t ODR;      // 0x14 - Output data
	volatile uint32_t BSRR;     // 0x18 - Bit set/reset
	volatile uint32_t LCKR;     // 0x1C - Lock
	volatile uint32_t AFR[2];   // 0x20-0x24 - Alternate function
} GPIO_TypeDef;

// ============================================================
// DIFFERENCE 4: RCC Register Structure
// STM32F103: Uses APB2ENR to enable GPIO clock
// STM32F446: Uses AHB1ENR to enable GPIO clock
// ============================================================
typedef struct {
	volatile uint32_t CR;           // 0x00
	volatile uint32_t PLLCFGR;      // 0x04
	volatile uint32_t CFGR;         // 0x08
	volatile uint32_t CIR;          // 0x0C
	volatile uint32_t AHB1RSTR;     // 0x10
	volatile uint32_t AHB2RSTR;     // 0x14
	volatile uint32_t AHB3RSTR;     // 0x18
	volatile uint32_t RESERVED0;    // 0x1C
	volatile uint32_t APB1RSTR;     // 0x20
	volatile uint32_t APB2RSTR;     // 0x24
	volatile uint32_t RESERVED1[2]; // 0x28-0x2C
	volatile uint32_t AHB1ENR;      // 0x30
	volatile uint32_t AHB2ENR;      // 0x34
	volatile uint32_t AHB3ENR;      // 0x38
	volatile uint32_t RESERVED2;    // 0x3C
	volatile uint32_t APB1ENR;      // 0x40
	volatile uint32_t APB2ENR;      // 0x44
} RCC_TypeDef;

#define RCC                     ((RCC_TypeDef*) RCC_BASE)
#define GPIOA                   ((GPIO_TypeDef*) GPIOA_BASE)


void delay_ms(int count) {
	for (volatile int i = 0; i < count; i++);
}

int main(void)
{
	RCC->AHB1ENR |= GPIOA_EN;

	GPIOA->MODER &= ~(3U<<10);
	GPIOA->MODER |=  (1U<<10);

	while (1) {
	    GPIOA->BSRR = LED_PIN;
	    delay_ms(1000000);

	    GPIOA->BSRR = (LED_PIN << 16);
	    delay_ms(1000000);
	}
}
