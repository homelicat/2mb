#define F_CPU 16000000UL  
#include <avr/io.h>      
#include <util/delay.h>  
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

uint8_t count = 0; 
uint8_t scancode = 0;
uint8_t last_scancode = 0;
uint8_t pre_last_scancode = 0;
uint8_t shift = 0;

uint8_t rx_buf[16];
uint8_t rx_last = 0;


uint8_t ascii_noshift[48] = {'`','1','2','3','4','5','6','7','8','9','0','-','=','q','w','e','r','t','y','u','i','o','p','[',']',' ','a','s','d','f','g','h','j','k','l',';','\'','z','x','c','v','b','n','m',',','.','/',' '};
uint8_t ascii_shift[48] = {' ','!','@','#','$','%','^','&','*','(',')','_','+','Q','W','E','R','T','Y','U','I','O','P','{','}','|','A','S','D','F','G','H','J','K','L',':','"','Z','X','C','V','B','N','M','<','>','?',' '};
uint8_t ascii_scancodes[48] = {0x0e,0x16,0x1e,0x26,0x25,0x2e,0x36,0x3d,0x3e,0x46,0x45,0x4e,0x55,    0x15,0x1d,0x24,0x2d,0x2c,0x35,0x3c,0x43,0x44,0x4d,0x54,0x5b,0x5d    ,0x1c,0x1b,0x23,0x2b,0x34,0x33,0x3b,0x42,0x4b,0x4c,0x52   ,0x1a,0x22,0x21,0x2a,0x32,0x31,0x3a,0x41,0x49,0x4a  ,0x29};
uint8_t keycodes[6] = {0x66,0x5a,0x75,0x72,0x6b,0x74};

uint8_t conv_scancode_ascii(uint8_t scancode, uint8_t is_shift)
{
	uint8_t result = 0;
	for (int i = 0; i < 48; i++)
	{
		if (scancode == ascii_scancodes[i])
		{
			if (is_shift)
			{
				result = ascii_shift[i];
			} else
			{
				result = ascii_noshift[i];
			}
		}
	}
	return result;
}

uint8_t conv_scancode_keycode(uint8_t scancode)
{
	uint8_t result = 0;
	for (int i = 0; i < 6; i++)
	{
		if (scancode == keycodes[i])
		{
			result = i+1;
		}
	}
	return result;
}

void lcd_half_byte(uint8_t data)
{
	PORTD = (PORTD | (data << 4)) & (~((~data) << 4));
	PORTD |= (1<<PD3);
	_delay_us(1);
	PORTD &= ~(1<<PD3);
}

void lcd_byte(uint8_t data)
{
	lcd_half_byte(data>>4);
	_delay_us(50); 
	lcd_half_byte((data<<4)>>4);
	_delay_us(50);
}

void lcd_init()
{
	_delay_ms(20);
	PORTB &= ~(1<<PB2);
	
	lcd_half_byte(0b00000011);
	_delay_ms(5);
	lcd_half_byte(0b00000011);
	_delay_ms(5);
	
	lcd_half_byte(0b00000011);
	_delay_ms(5);
	lcd_half_byte(0b00000010);
	_delay_ms(5);
	
	lcd_half_byte(0b00000010);
	_delay_ms(5);
	lcd_half_byte(0b00001000);
	_delay_ms(5);
	
	lcd_half_byte(0b00000000);
	_delay_ms(5);
	lcd_half_byte(0b00001110);
	_delay_ms(5);
	
	lcd_half_byte(0b00000001);
	_delay_ms(5);
	lcd_half_byte(0b00000100);
	_delay_ms(5);
	
	lcd_half_byte(0b00000000);
	_delay_ms(5);
	lcd_half_byte(0b00000001);
	_delay_ms(5);
}

void lcd_clear()
{
	PORTB &= ~(1<<PB2);
  lcd_byte(0b000000001);
  lcd_byte(0b000000010);
  _delay_ms(20);
}

void lcd_set(uint8_t offset)
{
	PORTB &= ~(1<<PB2);
  if (offset>31) return;
  uint8_t addr = (((int)offset/16)*64) + (offset%16) + 0b10000000;
  lcd_byte(addr);
}

void lcd_print(char symb)
{
	PORTB |= (1<<PB2);
   lcd_byte(symb);
}

void rx_push(uint8_t com, uint8_t data)
{
	for (int i = 0; i<16; i+=2)
	{
		if (rx_buf[i]==0)
		{
			rx_buf[i]=com;
			rx_buf[i+1]=data;
			return;
		}
	}
}

void init()
{
	DDRD |= (1 << PD3) + (1 << PD4) + (1 << PD5) + (1 << PD6) + (1 << PD7); 
	DDRB |= (1 << PB0) + (1 << PB2);
	DDRB &= ~(1<<PB1);
	DDRD &= ~(1<<PD2);
	PORTB = 0;
	PORTD = 0;
	sei();
	EICRA |= (1 << 1);
	EIMSK |= (1 << 0);
	for (int i = 0; i<16; i++) rx_buf[i]=0;
	
}

void init_uart()
{
	UBRR0H=0;
	UBRR0L=25;
	UCSR0B |= (1<<RXEN0)+(1<<TXEN0)+(1<<RXCIE0);
	UCSR0C |= (1<<UCSZ00)+(1<<UCSZ01);
}

void uart_send(uint8_t data) 
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void audio()
{
	
}

ISR(INT0_vect)
{
	cli();
	if((count==0) && ((PINB>>PB1)& 1u)) return;
	if ((count>0) && (count<9))
	{
		scancode |=(((PINB>>PB1)& 1u)<<(count-1));
	}
	count++;
	if(count==11)
	{
		
		if (((scancode==0x12)||(scancode==0x59))&&(last_scancode==0xf0))
		{
			shift=0;	
		}
		else if (((scancode==0x12)||(scancode==0x59)))
		{
			shift=1;
		}
		else if(last_scancode==0xf0)
		{
			if (conv_scancode_ascii(scancode,shift))
			{
				uart_send(6);
				uart_send(conv_scancode_ascii(scancode,shift)); 
			} else if(conv_scancode_keycode(scancode))
			{
				uart_send(1);
				uart_send(conv_scancode_keycode(scancode)); 
			}
		} 
		count = 0;
		last_scancode = scancode;
		scancode = 0;
	}
}

ISR(USART_RX_vect) 
{
	if (rx_last==0)
	{
		rx_last=UDR0;
	} else
	{
		rx_push(rx_last,UDR0);
		rx_last=0;
	}
}


void main() {                
	init();
	lcd_init();
	init_uart();
	while (1) 
	{
		for (int i = 0; i<16; i+=2)
		{
			if (rx_buf[i]!=0)
			{
				if(rx_buf[i]==02) lcd_print(rx_buf[i+1]);
				if(rx_buf[i]==03) lcd_set(rx_buf[i+1]);
				if(rx_buf[i]==04) lcd_clear();
				if(rx_buf[i]==05) audio();
				rx_buf[i]=0;
				rx_buf[i+1]=0;
			}
		}
	}
}
