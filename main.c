/*
 * FinalProject.c
 *
 * Created: 4/23/2024 8:18:56 PM
 * Author : Diego
 */ 
#define F_CPU 16 // in MHz

// sonic sensor pins
#define TRIGGER_PIN PC1
#define ECHO_PIN PC2
#define MAX_CM_DIST 10


#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

const char MenuMSG[] = "\r\nMenu: (L)CD, (A)DC, (P)ing USS\n\r";
const char InvalidCommandMSG[] = "\r\nInvalid Command Try Again...";
const char BannerMSG[] = "\r\nFinal Project - Diego, Jenerth, Roxana\n\r";

//external Assembly functions
void Mega328P_Init(void);
//void LCD_Init(void);			
//void UART_Init(void);
void UART_Clear(void);
void UART_Get(void);
void UART_Put(void);
void LCD_Write_Data(void);
void LCD_Write_Command(void);
void LCD_Read_Data(void);
void ADC_Get(void);

unsigned char ASCII;			//shared I/O variable with Assembly
unsigned char DATA;				//shared internal variable with Assembly
char HADC;						//shared ADC variable with Assembly
char LADC;						//shared ADC variable with Assembly

char volts[5];					//string buffer for ADC output
int Acc;						//Accumulator for ADC use

// function prototypes
void Timer0(double us);
void UART_Puts(const char *str);
void LCD_Puts(const char *str);
void LCD(void);
float calculateTemperature(int adcValue);
void ADConverter(void);
//void HCSR04_init(void);
int pingDistance(void);
void USS(void);
void Banner(void);
void Command(void);


int getPrescaler(){ 		// helper function to get the prescaler value of a timer
	switch (TCCR0B & 0x07) {  	// Mask with 0x07 to get the last three bits
        case 0x01: return 1;
        case 0x02: return 8;
        case 0x03: return 64;
        case 0x04: return 256;
        case 0x05: return 1024;
        default: return 0;  	// Clock is stopped
    }
}

int getPrescalerIndex(int prescaler){ // helper function to get the index of a prescaler value
	switch (prescaler){
		case 1: return 0;
		case 8: return 1;
		case 64: return 2;
		case 256: return 3;
		case 1024: return 4;
		default: return -1; // invalid prescaler value
	}
}

uint8_t setPrescalerBits(int prescaler){ // helper function to set the prescaler bits of a timer
	switch (prescaler){
		case 1: return (1 << CS00);
		case 8: return (1 << CS01);
		case 64: return (1 << CS01) | (1 << CS00);
		case 256: return (1 << CS02);
		case 1024: return (1 << CS02) | (1 << CS00);
		default: return 0; // invalid prescaler value
	}
}

void Timer1(double us){
    //allowed prescaler values
    int prescalers[] = {1, 8, 64, 256, 1024};
    int prescaler;
    //find the best prescaler value
    int i;
    for (i = 0; i < 5; i++){
        prescaler = prescalers[i];
        if (us * F_CPU <= 65536 * prescaler){ // found smallest prescaler value
            break;
        }
    }
    
    // Set up Timer1 for ctc mode with prescaler, and enable compare interrupt
	TCNT1 = 0;								// Clear the counter
	OCR1A = (us * F_CPU) / prescaler;		// Set the max compare value
	TIMSK1 = (1 << OCIE1A);					// Enable Timer1 compare interrupt

	// set prescaler value
	TCCR1B = setPrescalerBits(prescaler);



	/*
	TCCR1B = 0; 							// Normal mode
	TCNT1 = 0; 								// Clear the counter
    OCR1A = (us * F_CPU) / prescaler - 1; 	// Set the max compare value
    TIMSK1 = (1 << OCIE1A); 				// Enable Timer1 compare interrupt
    TCCR1B |= (i + 1); 						// set the prescaler value
	*/

    // Set up ECHO_PIN interrupt
    PCICR = (1 << PCIE1); 		// Enable pin change interrupt 1
    PCMSK1 = (1 << PCINT10); 	// Enable pin change interrupt for ECHO_PIN

    sei(); 						// enable global interrupts
}

// Timer1 interrupt for ECHO pin
ISR(PCINT1_vect){
    if (!(PINC & (1 << ECHO_PIN))){	// if ECHO pin is low
        TCCR1B = 0; // stop the timer
        
        // send time elapsed
        char msg[128];
        sprintf(msg, "time elapsed: %d\n", TCNT1 * getPrescaler() / F_CPU);
        UART_Puts(msg);
        
        // reset timer and clear flag
        //TCNT1 = 0;
        TIFR1 |= (1 << OCF1A);
    }
}

// Timer1 interrupt for timeout
ISR(TIMER1_COMPA_vect){
	TCCR1B &= ~(1 << CS10); // stop the timer
	// reset timer and clear flag
	TCNT1 = 0;
	TIFR1 |= (1 << OCF1A);

	// send timeout message
	char msg[128];
	sprintf(msg, "timeout, time elapsed: %d\n", TCNT1 * getPrescaler() / F_CPU);
	UART_Puts(msg);
}

// Timer1 overflow interrupt
ISR(TIMER1_OVF_vect) 
{	
    TCCR1B = 0; // stop the timer
    
    // send overflow message 
    char msg[128] = "Timer1 overflowed\n";
    UART_Puts(msg);
    
    // reset timer and clear flag
    //TCNT1 = 0;
    TIFR1 |= (1 << OCF1A);
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
	int x = 1;
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

void ADConverter(void)								// take in adc value and convert to temp
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

int pingDistance(void)						// helper function to time trigger ping and return distance
{
	UART_Puts("starting trigger ping\n");
	// Send a 10us pulse on the Trig pin
	PORTC |= (1 << TRIGGER_PIN);
	_delay_us(10);
	PORTC &= ~(1 << TRIGGER_PIN);

	// Measure the time the Echo pin stays high
	Timer1(1000000); // timeout value
	

	// Calculate the distance using the speed of sound (34300 cm/s), time measured and accounting for 2 trips
	float time = (TCNT1 * getPrescaler()) / F_CPU; // distance in cm
	float distance = (time * 34300) / 2;
	
	return distance;
}

void USS(void){
	int dist = pingDistance();
	char distAsStr[10]; // buffer to store distance as string

	sprintf(distAsStr, "distance = %d cm\n", dist); // actually convert to string
	
	//display to both UART and LCD
	UART_Puts(distAsStr);
	LCD_Puts(distAsStr);
	
	return;
}

void Banner(void)							// Display the Banner
{
	LCD_Puts(BannerMSG);
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
		case 'A' | 'a': ADConverter();
		break;
		case 'P' | 'p': USS();
		break;
		default: UART_Puts(InvalidCommandMSG);
		break;
		
	}
}


int main(void)
{
	Mega328P_Init();
	Banner();
	
	while (1){
		Command();
	}
	
}

