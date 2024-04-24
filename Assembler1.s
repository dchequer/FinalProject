
/*
 * Assembler1.s
 *
 * Created: 4/23/2024 8:28:37 PM
 *  Author: Diego
 */ 

.section ".data"					
.equ	DDRB,0x04					
.equ	DDRD,0x0A					
.equ	PORTB,0x05					
.equ	PORTD,0x0B					
.equ	U2X0,1						
.equ	UBRR0L,0xC4					
.equ	UBRR0H,0xC5					
.equ	UCSR0A,0xC0					
.equ	UCSR0B,0xC1					
.equ	UCSR0C,0xC2					
.equ	UDR0,0xC6					
.equ	RXC0,0x07					
.equ	UDRE0,0x05					
.equ	ADCSRA,0x7A					
.equ	ADMUX,0x7C					
.equ	ADCSRB,0x7B					
.equ	DIDR0,0x7E					
.equ	DIDR1,0x7F					
.equ	ADSC,6						
.equ	ADIF,4						
.equ	ADCL,0x78					
.equ	ADCH,0x79					
.equ	EECR,0x1F					
.equ	EEDR,0x20					
.equ	EEARL,0x21					
.equ	EEARH,0x22					
.equ	EERE,0						
.equ	EEPE,1						
.equ	EEMPE,2						
.equ	EERIE,3						

.global HADC				
.global LADC				
.global ASCII				
.global DATA				

.set	temp,0				

.section ".text"			
.global Mega328P_Init
Mega328P_Init:
		//***********************************************
		//initialize PB0(R*W),PB1(RS),PB2(E) as fixed cleared outputs
		ldi	r16,0x07		
		out	DDRB,r16		
		ldi	r16,0			
		out	PORTB,r16		
		//***********************************************
		//initialize UART, 8bits, no parity, 1 stop, 9600
		out	U2X0,r16		
		ldi	r17,0x0			
		ldi	r16,0x67		
		sts	UBRR0H,r17		
		sts	UBRR0L,r16		
		ldi	r16,24			
		sts	UCSR0B,r16		
		ldi	r16,6			
		sts	UCSR0C,r16		
		//************************************************
		//initialize ADC
		ldi r16,0x87		
		sts	ADCSRA,r16		
		ldi r16,0x40		
		sts ADMUX,r16		
		ldi r16,0			
		sts ADCSRB,r16		
		ldi r16,0xFE		
		sts DIDR0,r16		
		ldi r16,0xFF		
		sts DIDR1,r16		
		//************************************************
		ret					
	
.global LCD_Write_Command
LCD_Write_Command:
	call	UART_Off		
	ldi		r16,0xFF		
	out		DDRD,r16		
	lds		r16,DATA		
	out		PORTD,r16		
	ldi		r16,4			
	out		PORTB,r16		
	call	LCD_Delay		
	ldi		r16,0			
	out		PORTB,r16		
	call	LCD_Delay		
	call	UART_On			
	ret						

LCD_Delay:
	ldi		r16,0xFA		
D0:	ldi		r17,0xFF		
D1:	dec		r17				
	brne	D1				
	dec		r16				
	brne	D0				
	ret						

.global LCD_Write_Data
LCD_Write_Data:
	call	UART_Off		
	ldi		r16,0xFF		
	out		DDRD,r16		
	lds		r16,DATA		
	out		PORTD,r16		
	ldi		r16,6			
	out		PORTB,r16		
	call	LCD_Delay		
	ldi		r16,0			
	out		PORTB,r16		
	call	LCD_Delay		
	call	UART_On			
	ret						

.global LCD_Read_Data
LCD_Read_Data:
	call	UART_Off		
	ldi		r16,0x00		
	out		DDRD,r16		
	out		PORTB,4			
	in		r16,PORTD		
	sts		DATA,r16		
	out		PORTB,0			
	call	UART_On			
	ret						

.global UART_On
UART_On:
	ldi		r16,2				
	out		DDRD,r16			
	ldi		r16,24				
	sts		UCSR0B,r16			
	ret							

.global UART_Off
UART_Off:
	ldi	r16,0					
	sts UCSR0B,r16				
	ret							

.global UART_Clear
UART_Clear:
	lds		r16,UCSR0A			
	sbrs	r16,RXC0			
	ret							
	lds		r16,UDR0			
	rjmp	UART_Clear			

.global UART_Get
UART_Get:
	lds		r16,UCSR0A			
	sbrs	r16,RXC0			
	rjmp	UART_Get			
	lds		r16,UDR0			
	sts		ASCII,r16			
	ret							

.global UART_Put
UART_Put:
	lds		r17,UCSR0A			
	sbrs	r17,UDRE0			
	rjmp	UART_Put			
	lds		r16,ASCII			
	sts		UDR0,r16			
	ret							

.global ADC_Get
ADC_Get:
		ldi		r16,0xC7			
		sts		ADCSRA,r16			
A2V1:	lds		r16,ADCSRA			
		sbrc	r16,ADSC			
		rjmp 	A2V1				
		lds		r16,ADCL			
		sts		LADC,r16			
		lds		r16,ADCH			
		sts		HADC,r16			
		ret							

