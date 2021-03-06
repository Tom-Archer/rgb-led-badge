// Colour of each LED; hex digits represent RGB
volatile int Buffer[4] = { 0x000, 0x000, 0x000, 0x000 };

void DisplaySetup () {
  // ATtiny85
  // Set up Timer/Counter1 to multiplex the display
  TCCR1 = 1<<CTC1 | 7<<CS10;    // CTC mode; prescaler 64
  OCR1C = 24;                   // Divide by 25 -> 78Hz
  TIMSK = TIMSK | 1<<OCIE1A;    // Enable overflow interrupt

  // ATmega328P
  // Set up Timer/Counter2 to multiplex the display
  //TCCR2A = 0;                      // Reset TCCR0A
  //TCCR2B = 0;                      // Reset TCCR0B
  //TCNT2  = 0;                      // Reset TCNT2
  
  //TCCR2A |= (1 << WGM21);          // CTC mode
  //TCCR2B |= (5 << CS20);           // prescaler 128
  //OCR2A = 24;                      // Divide by 25 -> 78Hz
  //TIMSK2 |= (1 << OCIE2A);         // Enable overflow interrupt
} 

void setup() {
  DisplaySetup();
  //ADCSRA &= ~(1 << ADEN); // turn off adc
  PORTB |= (1 << PB4); // input
  DDRB &= ~(1 << PB4);
}

void DisplayNextRow() {
  static int cycle = 0;
  // Turn off the LED from the last cycle
  DDRB = DDRB & ~(1<<(cycle & 0x03));
  // Get the current cycle (0-63)
  cycle = (cycle + 1) & 0x3F;
  // Get the current LED number (0-3) 
  // Each LED is updated 16 times every 64 cycles
  int led = cycle & 0x03;
  // Get LED update number (0-15)
  int count = cycle>>2;
  // Read the RGB values from the buffer
  int rgb = Buffer[led];
  int r = rgb>>8 & 0x0F;
  int g = rgb>>4 & 0x0F;
  int b = rgb & 0x0F;
  // Determine whether each RGB value should be on (1) or off (0)
  int bits = (count < b) | (count < r)<<1 | (count < g)<<2;
  bits = bits + (bits & 0x07<<led);
  // Set anodes to output
  DDRB = (DDRB & 0xF0) | bits;
  // Set anodes to high
  PORTB = (PORTB & 0xF0) | bits;
  // Set cathode to low
  DDRB = DDRB | 1<<led;
}

// Timer/Counter2 interrupt - ATmega328P
//ISR(TIMER2_COMPA_vect) {
//  DisplayNextRow();
//}
// Timer/Counter1 interrupt - ATtiny85
ISR(TIM1_COMPA_vect) {
  DisplayNextRow();
}

unsigned int Step = 0;
int Mode = 0;
int Num_Modes = 10;
bool Switch_On = false;

void check_button_state()
{
  if ( !(PINB & (1 << PB4)) )
  {
    if (!Switch_On)
    {
      Switch_On = true;
      Step = 0;
      Mode++;
      Mode = Mode % Num_Modes; // constrain
    }
  }
  else
  {
    Switch_On = false;
  }
}

int red (unsigned int x) {
  int y = x % 48;
  if (y > 15) y = 31 - y;
  return max(y, 0);
}

int green (unsigned int x) { return red(x + 32); }
int blue (unsigned int x) { return red(x + 64); }

// These could probably operate from a single array, with some creative indexing
const int Flasher[2][16] PROGMEM = { 
  {0xF00, 0x000, 0xF00, 0x000, 0xF00, 0x000, 0xF00, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000 , 0x000},
  {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x00F, 0x000, 0x00F, 0x000, 0x00F, 0x000, 0x00F , 0x000},
  };
const int Flasher2[2][16] PROGMEM = { 
  {0xF00, 0x000, 0xF00, 0x000, 0xF00, 0x000, 0xF00, 0x000, 0x00F, 0x000, 0x00F, 0x000, 0x00F, 0x000, 0x00F , 0x000},
  {0x000, 0x00F, 0x000, 0x00F, 0x000, 0x00F, 0x000, 0x00F, 0x000, 0xF00, 0x000, 0xF00, 0x000, 0xF00, 0x000 , 0xF00},
  };

const byte Heart_Size = 24;
const byte Heart[Heart_Size] PROGMEM = {0x00, 0x02, 0x04, 0x08, 0x0A, 0x0C, 0x0E, 
                                        0x0B, 0x09, 0x06, 0x09, 0x0B, 0x0E, 
                                        0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02,
                                        0x00, 0x00, 0x00, 0x00, 0x00};

void loop () {
  check_button_state();
  switch (Mode)
  { 
    case 0:
      // Slow rainbow fade
      for (int i=0; i<2; i++) {
        Buffer[i] = red(Step + i*12)<<8 | green(Step + i*12)<<4 | blue(Step + i*12);
      }
      delay(200);
      break;
    case 1:
      // Fast rainbow fade
      for (int i=0; i<2; i++) {
        Buffer[i] = red(Step + i*12)<<8 | green(Step + i*12)<<4 | blue(Step + i*12);
      }
      delay(50);
      break;
    case 2:
      // Police flasher #1
      for (int i=0; i<2; i++) {
          Buffer[i] = pgm_read_word(&Flasher[i][(Step) % 16]);
      }
      delay(100);
      break;
    case 3:
      // Police flasher #2
      for (int i=0; i<2; i++) {
          Buffer[i] = pgm_read_word(&Flasher2[i][(Step) % 16]);
      }
      delay(100);
      break;
    case 4:
      // Version 1
      Buffer[0] = 0xFFF;
      Buffer[1] = 0xFFF;
      delay(200);
      break;
    case 5:
      // Red
      Buffer[0] = 0xF00;
      Buffer[1] = 0xF00;
      delay(200);
      break;
    case 6:
      // Green
      Buffer[0] = 0x0F0;
      Buffer[1] = 0x0F0;
      delay(200);
      break;
    case 7:
      // Blue
      Buffer[0] = 0x00F;
      Buffer[1] = 0x00F;
      delay(200);
      break;
    case 8:
      // Beating heart
      Buffer[0] = pgm_read_byte(&Heart[(Step % Heart_Size)]) << 8;
      Buffer[1] = Buffer[0];
      delay(100);
      break;
    case 9:
      // Slow red fade
      Buffer[0] = red(Step)<<8;
      Buffer[1] = Buffer[0];
      delay(200);
      break;
  }
  Step++;
  // Prevent overflow
  Step = Step % 32768;
}
