/**
 * In this project the arduino emulates two different devices.
 * The one device is an I2C light sensor and the other device
 * is a SPI PWM led.
 * The I2C address is 0x08 and the master just needs to read
 * a 16-bit word from the address, e.g.
 *  $ i2cget -y 0 0x08 0 w
 */

#include <Wire.h>
#include <SPI.h>

/**
 * 
 */

/* SPI slave */
volatile uint8_t spi_buf[2];
volatile byte spi_pos;
volatile boolean spi_rcv;

/* ADC value of the light sensor */
union un_adc_val {
  uint16_t val;
  uint8_t bytes[2];
};
union un_adc_val adc_val = {0};

int adc_pin = A3; // pin of the ADC channel that the light resistor is connected

int led_pin = 9;

void setup() {
  Serial.begin(9600);           //  setup debug serial
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onRequest(i2c_irq); // register event
  
  /* Setup SPI master */
  pinMode(MISO, OUTPUT);
  SPCR |= (1<<SPE)|(1<<SPIE); // turn on SPI in slave mode
  spi_pos = 0;   // buffer empty
  spi_rcv = false;
  // now turn on interrupts
  SPI.attachInterrupt();

  /* LED */
  pinMode(led_pin, OUTPUT);
  
  Serial.println("Device started...");
}


void loop() {
  delay(100);
  adc_val.val = analogRead(adc_pin);  // read the input pin
  if (spi_rcv) {
    uint16_t pwm_val = (spi_buf[0] << 8) | spi_buf[1];
    Serial.print("SPI: "); Serial.println(pwm_val, HEX);
    spi_pos = 0;
    spi_buf[0] = spi_buf[1] = 0;
    spi_rcv = false;
    analogWrite(led_pin, pwm_val/4);
  }
}


/**
 * Send the ADC value to the master
 */
void i2c_irq() {
  Wire.write(adc_val.bytes, 2); // respond with message of 6 bytes
  Serial.println(adc_val.val);           // debug value
}


/** 
 * SPI interrupt routine
 */
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // grab byte from SPI Data Register
  
  // add to buffer if room
  if (spi_pos <= sizeof spi_buf)
  {
    spi_buf[spi_pos++] = c;
    // example: newline means time to process buffer
    if (spi_pos >= 2)
      spi_rcv = true;
  }  // end of room available
}  // end of interrupt routine SPI_STC_vect
