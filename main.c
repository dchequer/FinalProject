/*
 * FinalProject.c
 *
 * Created: 4/23/2024 8:18:56 PM
 * Author : Diego
 */
#define F_CPU 16000000.0 // in hz

// sonic sensor pins
#define TRIGGER_PIN PC1
#define ECHO_PIN PC2
#define MAX_TIMEOUT 2000.0 // micro seconds ~ 2ms

#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

const char MenuMSG[] = "\r\nMenu: (L)CD, (A)DC, (P)ing USS\n\r";
const char InvalidCommandMSG[] = "\r\nInvalid Command Try Again...";
const char BannerMSG[] = "\r\nFinal Project - Diego, Jenerth, Roxana\n\r";

// external Assembly functions
void Mega328P_Init(void);
// void LCD_Init(void);
// void UART_Init(void);
void UART_Clear(void);
void UART_Get(void);
void UART_Put(void);
void LCD_Write_Data(void);
void LCD_Write_Command(void);
void LCD_Read_Data(void);
void ADC_Get(void);

unsigned char ASCII; // shared I/O variable with Assembly
unsigned char DATA;	 // shared internal variable with Assembly
char HADC;			 // shared ADC variable with Assembly
char LADC;			 // shared ADC variable with Assembly

char volts[5]; // string buffer for ADC output
int Acc;	   // Accumulator for ADC use

// function prototypes
void Timer0(double us);
void UART_Puts(const char *str);
void LCD_Puts(const char *str);
void LCD(void);
float calculateTemperature(int adcValue);
void ADConverter(void);
// void HCSR04_init(void);
float pingDistance(void);
void USS(void);
void Banner(void);
void Command(void);

float volatile prescaler;
/*
float getPrescaler(){
	int prescalerBits = TCCR1B & 0x7;	// mask last three bits
	switch (prescalerBits){
		case 0x1: return 1.0;
		case 0x2: return 8.0;
		case 0x3: return 64.0;
		case 0x4: return 256.0;
		case 0x5: return 1024.0;
		default: return 0.0;			// error in prescaler bits
	}

}
*/

void resetTimer1(void)
{
	// stop timer
	TCCR1B = 0x0;
	// disable timer overflow interrupt
	TIMSK1 &= ~(1 << TOIE1);
	// disable pin change interrupt for ECHO pin
	PCICR &= ~(1 << PCIE1);

	return;
}

void Timer1(float us)
{
	float s = us * pow(10, -6); // adjust us to microseconds

	int timerBits = 16;
	float C = pow(2, timerBits);
	// allowed prescaler values
	float prescalers[] = {1.0, 8.0, 64.0, 256.0, 1024.0};
	int i;
	// find the best prescaler value
	for (i = 0; i < 5; i++)
	{
		if (C - (F_CPU * s) / prescalers[i] >= 0)
		{
			prescaler = prescalers[i];
			break;
		}
	}

	// calculate timer count
	float timerCount = C - (F_CPU * s) / prescalers[i];

	// set timer count in register
	TCNT1 = 0;

	// set TCCR1A and TCCR1B registers for normal mode and prescaler
	TCCR1A = 0x0;	// normal mode
	TCCR1B = i + 1; // prescaler bits

	// enable timer overflow interrupt
	TIMSK1 = (1 << TOIE1);

	// enable pin change interrupt for ECHO pin
	PCICR = (1 << PCIE1);
	PCMSK1 = (1 << ECHO_PIN);

	while (TCCR1B != 0x0)
	{	
		UART_Puts("delaying...");
		// delay 10 us
		_delay_us(10);
		// manually trigger pin change interrupt
		PCIFR = (1 << PCIF1);
	} // wait for timer to finish
	return;
}

// Timer1 interrupt for ECHO pinF
ISR(PCINT1_vect)
{
	UART_Puts("echo interrupt \n\r");
	if (!(PINC & (1 << ECHO_PIN)))
	{ // if ECHO pin is low
		UART_Puts("AND ECHO WENT LOW \n\r");
		resetTimer1();
	}
}

// Timer1 overflow interrupt
ISR(TIMER1_OVF_vect)
{
	UART_Puts("overflow interrupt \n\r");
	resetTimer1();
}

void UART_Puts(const char *str) // Display a string in the PC Terminal Program
{
	while (*str)
	{
		ASCII = *str++;
		UART_Put();
	}
}

void LCD_Puts(const char *str) // Display a string on the LCD Module
{
	while (*str)
	{
		DATA = *str++;
		LCD_Write_Data();
	}
}

void LCD(void) // LCD Display
{
	DATA = 0x38; // 8 bit 2 line
	LCD_Write_Command();

	DATA = 0x0E; // display cursor on
	LCD_Write_Command();

	DATA = 0x01; // clear LCD
	LCD_Write_Command();

	LCD_Puts("test lcd");
}

float calculateTemperature(int adcValue) // helper function to calculate temperature from adc
{
	// Thermistor parameters
	float R0 = 10000.0; // Thermistor resistance at 25 degrees Celsius
	float T0 = 25.0;	// Temperature at which R0 is specified
	float B = 3950.0;	// Thermistor Beta value

	// ADC reference voltage and resolution
	float Vref = 3.3;	   // ADC reference voltage
	int resolution = 1024; // ADC resolution (10 bits)

	// Convert ADC value to voltage
	float voltage = (adcValue * Vref) / (float)resolution;

	// Calculate thermistor resistance
	float resistance = R0 * voltage / (Vref - voltage);

	// Calculate temperature in Kelvin using the Steinhart-Hart equation
	float tempKelvin = 1.0 / ((log(resistance / R0) / B) + (1.0 / (T0 + 273.15)));
	float temperature = tempKelvin - 273.15;

	return temperature;
}

void ADConverter(void) // take in adc value and convert to temp
{
	volts[0x1] = '.';
	volts[0x3] = ' ';
	volts[0x4] = 0;
	ADC_Get();
	Acc = (((int)HADC) * 0x100 + (int)(LADC)) * 0xA;
	volts[0x0] = 48 + (Acc / 0x7FE);
	Acc = Acc % 0x7FE;
	volts[0x2] = ((Acc * 0xA) / 0x7FE) + 48;
	Acc = (Acc * 0xA) % 0x7FE;
	if (Acc >= 0x3FF)
		volts[0x2]++;
	if (volts[0x2] == 58)
	{
		volts[0x2] = 48;
		volts[0x0]++;
	}
	int adcValue = (HADC << 8) | LADC;
	float temperature_Celsius = calculateTemperature(adcValue);
	unsigned int temp_integer = (int)temperature_Celsius;
	int temp_fractional = (int)((temperature_Celsius - temp_integer) * 100);
	sprintf(volts, "%d.%d degrees Celsius\n", temp_integer, temp_fractional);
	UART_Puts(volts);
}

/* already doing in assembly
void HCSR04_init(void){
	DDRC |= (1 << TRIGGER_PIN); // set PC1 as output
	DDRC &= ~(1 << ECHO_PIN);	// Set PC2 as input
}
*/

float pingDistance(void) // helper function to time trigger ping and return distance
{
	UART_Puts("starting trigger ping\n\r");
	// Send a 10us pulse on the Trig pin
	PORTC |= (1 << TRIGGER_PIN);
	_delay_us(10);
	PORTC &= ~(1 << TRIGGER_PIN);

	// Measure the time the Echo pin stays high
	Timer1(MAX_TIMEOUT); // timeout/overflow value

	// Calculate time passed
	float timeMS = ((TCNT1 * prescaler) / F_CPU) * 1000.0; // time in ms

	// Calculate distance using speed of sound (343 m/s) and accounting for return trip
	float distanceCM = (timeMS * 343.0 / 2.0) / 100.0; // distance in cm

	// debug outputs
	char buff[28];
	sprintf(buff, "Time passed: %d us \n\rPrescaler: %d \n\r", (int)(timeMS * 100.0), (int)prescaler); // time passed in microseconds
	UART_Puts(buff);
	UART_Puts("distance calculated \n\r");
	return distanceCM;
}

void USS(void)
{
	float distance = pingDistance();

	char buff[28];										// buffer to store distance as string
	sprintf(buff, "distance = %d cm\n", (int)distance); // meters

	// display to both UART and LCD
	UART_Puts(buff);
	LCD_Puts(buff);

	return;
}

void Banner(void) // Display the Banner
{
	LCD_Puts(BannerMSG);
	UART_Puts(BannerMSG);
	return;
}

void Command(void) // command interpreter
{
	UART_Puts(MenuMSG);
	ASCII = '\0';
	while (ASCII == '\0')
	{
		UART_Get();
	}
	switch (ASCII)
	{
	case 'L' | 'l':
		LCD();
		break;
	case 'A' | 'a':
		ADConverter();
		break;
	case 'P' | 'p':
		USS();
		break;
	default:
		UART_Puts(InvalidCommandMSG);
		break;
	}
}

int main(void)
{
	Mega328P_Init();
	Banner();
	sei();

	while (1)
	{
		Command();
	}
}
