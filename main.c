/*
 * FinalProject.c
 *
 * Created: 4/23/2024 8:18:56 PM
 * Author : Diego
 */ 
#define F_CPU 16000000

// sonic sensor pins
#define TRIGGER_PIN PC1;
#define ECHO_PIN PC2;
#define MAX_CM_DIST 10;


#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>

const char MenuMSG[] = "\r\nMenu: (L)CD, (A)DC\n\r";
const char InvalidCommandMSG[] = "\r\nInvalid Command Try Again...";
const char BannerMSG[] = "\r\nFinal Project - Diego, Jenerth, Roxana\n\r";

void LCD_Init(void);			//external Assembly functions
void UART_Init(void);
void UART_Clear(void);
void UART_Get(void);
void UART_Put(void);
void LCD_Write_Data(void);
void LCD_Write_Command(void);
void LCD_Read_Data(void);
void Mega328P_Init(void);
void ADC_Get(void);

unsigned char ASCII;			//shared I/O variable with Assembly
unsigned char DATA;				//shared internal variable with Assembly
char HADC;						//shared ADC variable with Assembly
char LADC;						//shared ADC variable with Assembly

char volts[5];					//string buffer for ADC output
int Acc;						//Accumulator for ADC use

void Banner(void)				// Display the Banner
{
	LCD_Puts(BannerMSG);
	return;
}

void UART_Puts(const char *str)				// Display a string in the PC Terminal Program
{
	while (*str)
	{
		ASCII = *str++;
		UART_Put();
	}
}

void LCD_Puts(const char *str)				// Display a string on the LCD Module
{
	while (*str)
	{
		DATA = *str++;
		LCD_Write_Data();
	}
}

void LCD(void)								// LCD Display
{
	int x = 100;
	DATA = 0x34;					
	LCD_Write_Command();
	DATA = 0x08;					
	LCD_Write_Command();
	DATA = 0x02;						
	LCD_Write_Command();
	DATA = 0x06;						
	LCD_Write_Command();
	DATA = 0x0f;						
	LCD_Write_Command();

	while (x != 0){
		
		DATA = 0x18;						//shift complete to the left
		LCD_Write_Command();
		LCD_Puts("Hello");
		_delay_us(1000);
		x--;
	}
}

float calculateTemperature(int adcValue)	// helper function to calculate temperature from adc
{
	// Thermistor parameters
	float R0 = 10000.0;   // Thermistor resistance at 25 degrees Celsius
	float T0 = 25.0;      // Temperature at which R0 is specified
	float B = 3950.0;     // Thermistor Beta value

	// ADC reference voltage and resolution
	float Vref = 3.3;     // ADC reference voltage
	int resolution = 1024;  // ADC resolution (10 bits)

	// Convert ADC value to voltage
	float voltage = (adcValue * Vref) / (float)resolution;

	// Calculate thermistor resistance
	float resistance = R0 * voltage / (Vref - voltage);
	
	// Calculate temperature in Kelvin using the Steinhart-Hart equation
	float tempKelvin = 1.0 / ((log(resistance / R0) / B) + (1.0 / (T0 + 273.15)));
	float temperature = tempKelvin - 273.15;
	
	return temperature;
}

void ADC(void)								// take in adc value and convert to temp
{	
	int adcValue = (HADC << 8) | LADC;
	
	float temperature_Celsius = calculateTemperature(adcValue);
	unsigned int temp_integer = (int)temperature_Celsius;
	int temp_fractional = (int)((temperature_Celsius - temp_integer)*100);
	
	sprintf(volts, "%d.%d degrees Celsius\n", temp_integer, temp_fractional);
	UART_Puts(volts);
}

/* already doing in assembly
void HCSR04_init(void){
	DDRC |= (1 << TRIGGER_PIN); // set PC1 as output
	DDRC &= ~(1 << ECHO_PIN);	// Set PC2 as input
}
*/

int pingDistance(void)
{
	// Send a 10us pulse on the Trig pin
	PORTC |= (1 << TRIGGER_PIN);
	_delay_us(10);
	PORTC &= ~(1 << TRIGGER_PIN);

	// Wait for the Echo pin to go high
	while (!(PINC & (1 << ECHO_PIN)));

	// Measure the time the Echo pin stays high
	int count = 0;
	while (PINC & (1 << ECHO_PIN)) {
		count++;
		_delay_us(1);
	}

	return count;
}

void USS(void){
	int dist = pingDistance();
	char distAsStr[10]; // buffer to store distance as string

	sprintf(distAsStr, "%d cm\n", dist); // actually convert to string
	
	//display to both UART and LCD
	UART_Puts(distAsStr);
	LCD_Puts(distAsStr);
	
	return;
}

void Command(void)							// command interpreter
{
	UART_Puts(MenuMSG);
	ASCII = '\0';
	while (ASCII == '\0')
	{
		UART_Get();
	}
	switch (ASCII)
	{
		case 'L' | 'l': LCD();
		break;
		case 'A' | 'a': ADC();
		break;
		case 'P' | 'p': USS();
		break;
		default: UART_Puts(InvalidCommandMSG);
		break;
		
	}
}


int main(void)
{
	Banner();
	
	while (1){
		Command();
	}
	
}

