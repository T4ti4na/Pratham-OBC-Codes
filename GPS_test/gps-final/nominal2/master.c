/**
 * @file master.c
 * @brief Main file for master uC.
 * 
 * Contains the scheduler, preflight check routines, high level task blocks like power, control and communication.
 */
#define F_CPU 8000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

///4-byte buffer for the GPS reading
volatile static uint32_t buffer = 0;
///Position variables for the data in GPS structure
volatile static uint8_t pos = 0xFF,vel = 0xFF,dop = 0xFF,geo = 0xFF, time = 0xFF;
///Variables to check whether the message has ended
volatile static uint8_t last_byte, message_end;
//volatile static uint16_t pdop = 0xFFFF;
volatile struct state Current_state;
/// Captures GPS reading
struct GPS_reading{// x y z vx vy vz lat lon alt have been changed from int 32 to uint 32 for purpose of oils
  ///* Position data bytes: Long(4) Unit(mm) ECEF Frame
  int32_t x;
  int32_t y;
  int32_t z;
  
  ///* Velocity data bytes: Long(4) Unit(mm/s) ECEF Frame
  int32_t v_x;
  int32_t v_y;
  int32_t v_z;
  
  ///* Latitude Longitude Altitude
  int32_t lat;
  int32_t lon;
  int32_t alt;
  
  ///* Time data HH:MM:SS DD/MM/YYYY
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  
  uint8_t date;
  uint8_t month;
  uint16_t year;
  uint16_t pdop;
  
  ///* Stores the time since last GPS reading
  uint16_t time_since_reading;
};

struct state{
	struct GPS_reading gps;
};

volatile struct GPS_reading gps;

char arrayx[40];

char arrayy[40];

char arrayz[40];

uint8_t receivedbyte;


 void init_UART_GPS(void)
{

  UCSR0A = 0;
  UCSR0B = 0;
  UCSR0C = 0;
  
  ///Double Baud Rate
  UCSR0A |= _BV(U2X0);
  ///Enable Reception
  UCSR0B |= _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
  ///8-bit Data Byte, 2 Stop bits
  UCSR0C |= _BV(USBS0) | _BV(UCSZ01) | _BV(UCSZ00);
  ///Set Baud Rate to 9600
  UBRR0L = 103;
  UBRR0H = 0;
  
  Current_state.gps.x=1;
  Current_state.gps.y=1;
  Current_state.gps.z=1;
  Current_state.gps.v_x=1;
  Current_state.gps.v_y=1;
  Current_state.gps.v_z=1;
  Current_state.gps.lat=1;
  Current_state.gps.lon=1;
  Current_state.gps.alt=1;
  Current_state.gps.hours=1;
  Current_state.gps.minutes=1;
  Current_state.gps.seconds=1;
  Current_state.gps.date=1;
  Current_state.gps.month=1;
  Current_state.gps.year=1;
  Current_state.gps.pdop=1;
}


void transmit_UART0(uint8_t data)
{
  //* Wait for empty transmit buffer 
  while ( !(UCSR0A & (_BV(UDRE0))) );
  //* Start transmission
  UDR0 = data;
}


volatile uint8_t crap = 0;

// @brief Interrupt on receiving a byte through UART GPS
 
ISR(USART0_RX_vect)
{ 
	
  PORTA ^= 0xf0;
  
  ///Buffer the Received Byte
  last_byte = UDR0;
  
  ///Put the received byte in the last 4-bytes buffer
  buffer = buffer << 8;
  buffer &= 0xFFFFFF00;
  buffer |= (uint32_t) last_byte;
  
  ///Check if the last byte was for position
  if(pos < 12)
  {
    
    if(pos<4)
	{
		Current_state.gps.x = Current_state.gps.x >> 8;
		Current_state.gps.x &= 0x00FFFFFF;
        Current_state.gps.x |= ((uint32_t) last_byte)<<24;
		
	}
	else if (pos<8)
	{
		Current_state.gps.y = Current_state.gps.y >> 8;
		Current_state.gps.y &= 0x00FFFFFF;
		Current_state.gps.y |= ((uint32_t) last_byte)<<24;
	}
	
	else if(pos<12)
	{
		Current_state.gps.z = Current_state.gps.z >> 8;
		Current_state.gps.z &= 0x00FFFFFF;
		Current_state.gps.z |= ((uint32_t) last_byte)<<24;
		
	}
    ///* Increment position and terminate it if full
    pos++;
    if(pos == 12)
	{
      pos = 0xFF;
	   }
	   
  }
  
  ///Check if the last byte was for velocity
  if(vel < 12)
  {
	  
    if(vel < 4)
	{
		Current_state.gps.v_x = Current_state.gps.v_x >> 8;
		Current_state.gps.v_x &= 0x00FFFFFF;
        Current_state.gps.v_x |= ((uint32_t) last_byte)<<24;
		
	}
	else if(vel < 8)
	{
		
		Current_state.gps.v_y = Current_state.gps.v_y >> 8;
		Current_state.gps.v_y &= 0x00FFFFFF;
        Current_state.gps.v_y |= ((uint32_t) last_byte)<<24;
	}
	else if(vel < 12)
	{
		Current_state.gps.v_z = Current_state.gps.v_z >> 8;
		Current_state.gps.v_z &= 0x00FFFFFF;
        Current_state.gps.v_z |= ((uint32_t) last_byte)<<24;
	}

	vel++;
    if(vel == 12)
     {
		  vel = 0xFF;
	 }
  }
  
  ///Check if the last byte was for PDOP
  if(dop < 4)
  {
    if(dop >= 2)
    {
		 Current_state.gps.pdop =  Current_state.gps.pdop >> 8;
		 Current_state.gps.pdop &= 0x00FF;
         Current_state.gps.pdop |= ((uint16_t) last_byte)<<8;
     }

	dop++;
    if(dop == 4)
      dop = 0xFF;
  }
  
  ///Check if the last byte was for Geodetic position
  if(geo < 16)
  {
	if(geo < 4);  
    else if(geo < 8)
	{
		Current_state.gps.lat = Current_state.gps.lat >> 8;
		Current_state.gps.lat &= 0x00FFFFFF;
		Current_state.gps.lat |= ((uint32_t) last_byte)<<24;
	}
	else if(geo < 12)
	{
		Current_state.gps.lon = Current_state.gps.lon >> 8;
		Current_state.gps.lon &= 0x00FFFFFF;
		Current_state.gps.lon |= ((uint32_t) last_byte)<<24;
	}
	else if(geo < 16 )
	{
		Current_state.gps.alt = Current_state.gps.alt >> 8;
		Current_state.gps.alt &= 0x00FFFFFF;
		Current_state.gps.alt |= ((uint32_t) last_byte)<<24;
	}
	    
    geo++;
    if(geo == 16)
	{
      geo = 0xFF;
	}
  }
  
  ///Check if the last byte was for Time
  if(time < 7)
  {
	  
	  if(time == 0)
	  {
		  Current_state.gps.hours = last_byte;
	  }
	  else if(time == 1)
	  {
		  Current_state.gps.minutes = last_byte;
	  }
	  else if(time == 2)
	  {
		  Current_state.gps.seconds = last_byte;
	  }
	  else if(time == 3)
	  {
		  Current_state.gps.date = last_byte;
	  }else if(time == 4)
	  {
		  Current_state.gps.month = last_byte;
	  }else if(time > 4)
	  {
		  Current_state.gps.year = Current_state.gps.year >> 8;
		  Current_state.gps.year &= 0x00FF;
		  Current_state.gps.year |= ((uint16_t) last_byte)<<8;

	  }
    
    time++;
    if(time == 7)
    {
      time = 0xFF;
      ///* * The Entire message has been read
      message_end = 1;
	  
	 //  UCSR0B &= ~(_BV(RXCIE0)); //remember this change iterrupt off
    }
  }
  
  ///Check the buffer against message ID's
  ///Position
  if(buffer == 0x3F3F04AC)
  {
    message_end = 0;
    pos = 0;
  }
  ///Velocity
  else if(buffer == 0x3F3F05AC)
    vel = 0;
  ///DOP
  else if(buffer == 0x3F3F0BAC)
    dop = 0;
  ///Geodetic System
  else if(buffer == 0x3F3F0EAC)
    geo = 0;
  ///Time
  else if(buffer == 0x3F3F0FAC)
    time = 0;
  
  ///Check if PDOP < 500(factor 100), shut off the interrupt and the GPS device
 /* if(( Current_state.gps.pdop < 500000)  && message_end)//&& ( Current_state.gps.pdop!=0)
  {
    //GPS_done = 1;
    
    // Switch off the interrupt
   // UCSR0B &= ~(_BV(RXCIE0));
    // Switch off the GPS device
    //power_down_peripheral(PGPS);
    
     Current_state.gps.pdop = 0xFFFF;
    message_end = 0;
    buffer = 0;
  }*/
   if( message_end==1)//&& ( Current_state.gps.pdop!=0)
  {
    
    cli();          //disable interrupt
       
     //Current_state.gps.pdop = 0xFFFF;
    message_end = 0;
    buffer = 0;
	crap = 1;     //flag set to 1 to enable transmit when message ends
	
  }
 
}

	
int main(void)
{
	init_UART_GPS();
	sei();
	
	uint8_t a,b,c,d;
	while (1)
	{
		if (crap==1)
		{
	
		a = (uint8_t)Current_state.gps.x;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.x>>8);
		transmit_UART0(b);
		c = (uint8_t)(Current_state.gps.x>>16);
		transmit_UART0(c);
		d = (uint8_t)(Current_state.gps.x>>24);
		transmit_UART0(d);
	
		a = (uint8_t)Current_state.gps.y;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.y>>8);
		transmit_UART0(b);
		c = (uint8_t)(Current_state.gps.y>>16);
		transmit_UART0(c);
		d = (uint8_t)(Current_state.gps.y>>24);
		transmit_UART0(d);
	
		a = (uint8_t)Current_state.gps.z;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.z>>8);
		transmit_UART0(b);
		c = (uint8_t)(Current_state.gps.z>>16);
		transmit_UART0(c);
		d = (uint8_t)(Current_state.gps.z>>24);
		transmit_UART0(d);
	
		a = (uint8_t)Current_state.gps.pdop;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.pdop>>8);
		transmit_UART0(b);
	
		a = (uint8_t)Current_state.gps.lat;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.lat>>8);
		transmit_UART0(b);
		c = (uint8_t)(Current_state.gps.lat>>16);
		transmit_UART0(c);
		d = (uint8_t)(Current_state.gps.lat>>24);
		transmit_UART0(d);
	
		a = (uint8_t)Current_state.gps.lon;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.lon>>8);
		transmit_UART0(b);
		c = (uint8_t)(Current_state.gps.lon>>16);
		transmit_UART0(c);
		d = (uint8_t)(Current_state.gps.lon>>24);
		transmit_UART0(d);
	
		a = (uint8_t)Current_state.gps.alt;
		transmit_UART0(a);
		
		b = (uint8_t)(Current_state.gps.alt>>8);
		transmit_UART0(b);
		c = (uint8_t)(Current_state.gps.alt>>16);
		transmit_UART0(c);
		d = (uint8_t)(Current_state.gps.alt>>24);
		transmit_UART0(d);
	
		crap =0;    //set flag to zero and enable interrupt after transmission
		sei();
		}
	}
	return 0;
}