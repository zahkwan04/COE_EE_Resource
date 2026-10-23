// ============================================================
// AVR Register-Level Blink — no Arduino digitalWrite()
// Target: ATmega328P (Arduino Uno)
// ============================================================
//
// The ATmega328P has three I/O ports: B, C, D.
// Each port has three 8-bit registers:
//
//   DDRx  — Data Direction Register  (1 = output, 0 = input)
//   PORTx — Output value             (1 = HIGH,    0 = LOW)
//   PINx  — Input read               (reads the physical pin state)
//
// On the Uno:
//   PB5 (pin 13) → built-in LED
//   PD2..PD7     → digital pins 2..7
//   PB0..PB5     → digital pins 8..13
//
// We'll drive the built-in LED (PB5) and an external LED on PD4.

#include <avr/io.h>
#include <util/delay.h>

// ------------------------------------------------------------
// Bit position definitions (like a mini datasheet)
// ------------------------------------------------------------
#define LED_BUILTIN_BIT   5   // PB5 = Arduino pin 13
#define EXT_LED_BIT       4   // PD4 = Arduino pin 4

// ------------------------------------------------------------
// The four idioms, applied to real registers
// ------------------------------------------------------------

static inline void clear_bit(volatile uint8_t *reg, uint8_t bit){
  *reg &= ~(1<<bit);
  
}

static inline void set_bit(volatile uint8_t *reg, uint8_t bit){
  *reg |= (1<<bit);
}

static inline void toggle_bit(volatile uint8_t *reg, uint8_t bit){
  *reg ^= (1<<bit);
}

static inline uint8_t check_bit_set(volatile uint8_t *reg, uint8_t bit){
  return *reg & (1<<bit);
  // or (*fake_reg & (1 << 5)) != 0;
}

// ============================================================
// setup()
// ============================================================
void setup(void) {
    // --------------------------------------------------------
    // Configure PB5 as OUTPUT
    // DDRB bit 5 = 1 → output
    // --------------------------------------------------------
    DDRB |= (1 << LED_BUILTIN_BIT);

    // --------------------------------------------------------
    // Configure PD4 as OUTPUT
    // DDRD bit 4 = 1 → output
    // --------------------------------------------------------
    DDRD |= (1 << EXT_LED_BIT);

    // --------------------------------------------------------
    // Optional: enable pull-up on PD2 (as a demo of input config)
    // DDRD bit 2 stays 0 (input), PORTD bit 2 = 1 enables pull-up
    // --------------------------------------------------------
    PORTD |= (1 << 2);
}

// ============================================================
// loop()
// ============================================================
void loop(void) {
    // Method 1: Blink using SET and CLEAR
    set_bit(&PORTB, LED_BUILTIN_BIT);
    PORTD |= (1 << EXT_LED_BIT);       // external LED on
    _delay_ms(250);

    clear_bit(&PORTB, LED_BUILTIN_BIT);
    PORTD &= ~(1 << EXT_LED_BIT);      // external LED off
    _delay_ms(250);

    // Method 2: Blink using TOGGLE (simpler, one line)
    // This does exactly the same thing as on/off above,
    // but in a single statement per LED.
    for (int i = 0; i < 4; ++i) {
        toggle_bit(&PORTB, LED_BUILTIN_BIT);                  // toggle PB5
        PORTD ^= (1 << EXT_LED_BIT);    // toggle PD4
        _delay_ms(150);
    }

    // Method 3: Read the pin state and act on it
    if (check_bit_set(&PINB, LED_BUILTIN_BIT)) {
        // If the built-in LED is currently on, turn both off
        PORTB &= ~(1 << LED_BUILTIN_BIT);
        PORTD &= ~(1 << EXT_LED_BIT);
        _delay_ms(400);
    }
}