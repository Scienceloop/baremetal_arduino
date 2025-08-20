#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define MAX_SOFT_PWM_CHANNELS 8

typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    uint8_t pin;
    uint8_t duty;  // 0-255
} SoftPWM_Channel;

SoftPWM_Channel softPWM[MAX_SOFT_PWM_CHANNELS];
uint8_t softPWM_count = 0;
volatile uint8_t pwm_counter = 0;

// ============================
// Hardware PWM Initialization
// ============================
void PWM_Hardware_Init() {
    // Timer0: OC0A (D6), OC0B (D5)
    DDRD |= (1 << PD6) | (1 << PD5);
    TCCR0A = (1 << WGM00) | (1 << WGM01) | (1 << COM0A1) | (1 << COM0B1);
    TCCR0B = (1 << CS01); // Prescaler 8
    OCR0A = 0; OCR0B = 0;

    // Timer1: OC1A (D9), OC1B (D10)
    DDRB |= (1 << PB1) | (1 << PB2);
    TCCR1A = (1 << WGM10) | (1 << WGM11) | (1 << COM1A1) | (1 << COM1B1);
    TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler 8
    OCR1A = 0; OCR1B = 0;

    // Timer2: OC2A (D11), OC2B (D3)
    DDRB |= (1 << PB3);
    DDRD |= (1 << PD3);
    TCCR2A = (1 << WGM20) | (1 << WGM21) | (1 << COM2A1) | (1 << COM2B1);
    TCCR2B = (1 << CS21); // Prescaler 8
    OCR2A = 0; OCR2B = 0;
}

// ============================
// Software PWM Functions
// ============================
void PWM_Software_Init() {
    TCCR2A = 0;             // Normal mode
    TCCR2B = (1 << CS21);   // Prescaler 8
    TIMSK2 = (1 << TOIE2);  // Enable overflow interrupt
}

void SoftPWM_AddChannel(volatile uint8_t *ddr, volatile uint8_t *port, uint8_t pin) {
    if(softPWM_count < MAX_SOFT_PWM_CHANNELS) {
        softPWM[softPWM_count].ddr = ddr;
        softPWM[softPWM_count].port = port;
        softPWM[softPWM_count].pin = pin;
        softPWM[softPWM_count].duty = 0;
        *(ddr) |= (1 << pin);
        softPWM_count++;
    }
}

void SoftPWM_SetDuty(uint8_t channel, uint8_t duty) {
    if(channel < softPWM_count)
        softPWM[channel].duty = duty;
}

// ============================
// Timer2 Overflow ISR for Software PWM
// ============================
ISR(TIMER2_OVF_vect) {
    pwm_counter++;
    for(uint8_t i = 0; i < softPWM_count; i++) {
        if(pwm_counter < softPWM[i].duty)
            *(softPWM[i].port) |= (1 << softPWM[i].pin);
        else
            *(softPWM[i].port) &= ~(1 << softPWM[i].pin);
    }
}

// ============================
// Main
// ============================
int main() {
    cli(); // Disable global interrupts

    PWM_Hardware_Init();
    PWM_Software_Init();

    // Add software PWM channels (example: PB4, PB5, PB0)
    SoftPWM_AddChannel(&DDRB, &PORTB, PB4);
    SoftPWM_AddChannel(&DDRB, &PORTB, PB5);
    SoftPWM_AddChannel(&DDRB, &PORTB, PB0);

    sei(); // Enable global interrupts

    while(1) {
        // Hardware PWM duty cycles
        OCR0A = 128; // D6 ~50%
        OCR0B = 64;  // D5 ~25%
        OCR1A = 200; // D9 ~78%
        OCR1B = 100; // D10 ~39%
        OCR2A = 150; // D11 ~58%
        OCR2B = 80;  // D3 ~31%

        // Software PWM duty cycles
        SoftPWM_SetDuty(0, 128); // PB4 ~50%
        SoftPWM_SetDuty(1, 200); // PB5 ~78%
        SoftPWM_SetDuty(2, 50);  // PB0 ~20%
    }
}
