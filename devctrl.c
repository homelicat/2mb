#include <stdint.h>
#include "lib/stm8.h"

#define LCD_RS PIN3
#define LCD_E PIN4
#define LCD_4 PIN5
#define LCD_5 PIN6
#define LCD_6 PIN7
#define LCD_7 PIN2
#define F_CPU 16000000UL
#define UART_BAUD 38400
#define UART_RX PIN6
#define UART_TX PIN5

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

//800000 = 1 s
//800 = 1 ms
//0.8 = 1 us
//8 = 10 us

void delay(unsigned long count) {
    while (count--);
}

uint8_t conv_scancode_ascii(uint8_t scancode, uint8_t is_shift)
{
	uint8_t result = 0;
	int i;
	
	for (i = 0; i < 48; i++)
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
	int i;
	
	for (i = 0; i < 6; i++)
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
	PD_ODR &= ~(LCD_7);
	PD_ODR |= (data>>3)<<2;
	
	PC_ODR &= ~(LCD_4+LCD_5+LCD_6);
	PC_ODR |= (data<<5);
	
	PC_ODR |= LCD_E;
	delay(1);
	PC_ODR &= ~LCD_E;
}

void lcd_byte(uint8_t data)
{
	lcd_half_byte(data>>4);
	delay(8*5); 
	
	lcd_half_byte((data<<4)>>4);
	delay(8*5);
}

void lcd_init()
{
	delay(20*800);
	PC_ODR &= ~LCD_RS;
	
	lcd_half_byte(0b00000011);
	delay(5*800);
	lcd_half_byte(0b00000011);
	delay(5*800);
	
	lcd_half_byte(0b00000011);
	delay(5*800);
	lcd_half_byte(0b00000010);
	delay(5*800);
	
	lcd_half_byte(0b00000010);
	delay(5*800);
	lcd_half_byte(0b00001000);
	delay(5*800);
	
	lcd_half_byte(0b00000000);
	delay(5*800);
	lcd_half_byte(0b00001110);
	delay(5*800);
	
	lcd_half_byte(0b00000001);
	delay(5*800);
	lcd_half_byte(0b00000100);
	delay(5*800);
	
	lcd_half_byte(0b00000000);
	delay(5*800);
	lcd_half_byte(0b00000001);
	delay(5*800);
}

void lcd_clear()
{
  PC_ODR &= ~LCD_RS;
  lcd_byte(0b000000001);
  lcd_byte(0b000000010);
  delay(20*800);
}

void lcd_set(uint8_t offset)
{
	uint8_t addr;
	
	PC_ODR &= ~LCD_RS;
	if (offset>31) return;
	addr = (((int)offset/16)*64) + (offset%16) + 0b10000000;
	lcd_byte(addr);
}

void lcd_print(char symb)
{
	PC_ODR |= LCD_RS;
	lcd_byte(symb);
}

void rx_push(uint8_t com, uint8_t data)
{
	int i;
	
	for (i = 0; i<16; i+=2)
	{
		if (rx_buf[i]==0)
		{
			rx_buf[i]=com;
			rx_buf[i+1]=data;
			return;
		}
	}
}

void uart_int(void) __interrupt(18) 
{
    if (rx_last==0)
	{
		rx_last=UART1_DR;
	} else
	{
		rx_push(rx_last,UART1_DR);
		rx_last=0;
	}
}

void init_uart()
{
	UART1_CR2 |= UART_CR2_TEN; // Transmitter enable
    UART1_CR2 |= UART_CR2_REN;
    UART1_CR2 |= UART_CR2_RIEN; 
    UART1_CR3 &= ~(UART_CR3_STOP1 | UART_CR3_STOP2);
    UART1_BRR2 = (F_CPU/UART_BAUD) & 0x000F;
	UART1_BRR2 |= (F_CPU/UART_BAUD) >> 12;
	UART1_BRR1 = ((F_CPU/UART_BAUD) >> 4) & 0x00FF;
}

void uart_send(uint8_t data) 
{
	while(!(UART1_SR & UART_SR_TXE));
	UART1_DR = data;
}


void init()
{
	int i = 0;
	
	
    CLK_CKDIVR = 0;
    
	PC_DDR |= LCD_4 + LCD_5 + LCD_6 + LCD_E + LCD_RS;
    PC_CR1 |= LCD_4 + LCD_5 + LCD_6 + LCD_E + LCD_RS;
    
    PD_DDR |= LCD_7;
    PD_CR1 |= LCD_7;
    
    PD_DDR |= (1<<5);
    
    //PB_DDR |= (1<<5);
    //PB_DDR |= (1<<5);
    //PB_ODR |= (1<<5);
    
    PB_CR1 |= (1<<4);
    PB_CR2 |= (1<<4);
    
    
    EXTI_CR1 = (1<<3);
    
    enableInterrupts()
    
    for (i = 0; i<16; i++) rx_buf[i]=0;
	
}

void key_int(void) __interrupt(4) 
{
	if((count==0) && ((PA_IDR>>3)& 1u)) return;
	if ((count>0) && (count<9))
	{
		scancode |=(((PA_IDR>>3)& 1u)<<(count-1));
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


int main(void)
{
	int i;
    
    init();
    lcd_init();
    init_uart();
    
    while (1) 
	{
		for (i = 0; i<16; i+=2)
		{
			if (rx_buf[i]!=0)
			{
				if(rx_buf[i]==02) lcd_print(rx_buf[i+1]);
				if(rx_buf[i]==03) lcd_set(rx_buf[i+1]);
				if(rx_buf[i]==04) lcd_clear();
				rx_buf[i]=0;
				rx_buf[i+1]=0;
			}
		}
	}
}
