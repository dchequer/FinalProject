/*
 * FinalProject.c
 *
 * Created: 4/23/2024 8:18:56 PM
 * Author : Diego
 */ 
#define F_CPU 16000000

// sonic sensor pins
#define TRIGGER_PIN = 0x0;
#define ECHO_PIN = 0x1;
#define MAX_CM_DIST = 10;

// lcd pins
#define RS_PIN = 0x2;
#define RW_PIN = 0x3;
#define ENABLE_PIN = 0x4;



#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
#include "NewPing.cpp"

const char MenuMSG[] = "\r\nMenu: (L)CD, (A)DC\n\r";
const char InvalidCommandMSG[] = "\r\nInvalid Command Try Again...";

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

void UART_Puts(const char *str)				//Display a string in the PC Terminal Program
{
	while (*str)
	{
		ASCII = *str++;
		UART_Put();
	}
}

void LCD_Puts(const char *str)				//Display a string on the LCD Module
{
	while (*str)
	{
		DATA = *str++;
		LCD_Write_Data();
	}
}

void LCD(void)								//LCD Display
{
	int x = 100;
	int i = 0;
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
		default: UART_Puts(InvalidCommandMSG);
		break;
		
	}
}

int main(void)
{
	NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_CM_DIST);
	
}

