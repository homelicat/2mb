#define F_CPU 16000000UL  
#include <avr/io.h>        
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t rx_buf[16];
uint8_t rx_last = 0;
uint8_t led_status = 1;

char welcome_str[] = "2MB 3.3 Hmundik";
char basic_str[] = " BF";
char reboot_str[] = " Reboot";
char led_str[] = " Led";
char led_on_off[] = "ON   OFF";
char led_on[] = "<==";
char led_off[] = "==>";
char bf_error[] = "Error";
char bf_complete[] = "Complete";


char text[1024];
uint8_t cpu[256] = {0};


void text_clear()
{
	for (int i = 0;i<sizeof(text);i++)
	{
		text[i]='\n';
	}
}


void text_add(uint8_t chr, uint16_t offset)
{
	for (int i = sizeof(text)-2; i!=offset;i--)
	{
		text[i]=text[i-1];
	}
	text[offset]=chr;
}

void text_rem(uint16_t offset)
{
	for (int i = offset; i<sizeof(text)-1;i++)
	{
		text[i]=text[i+1];
	}
}

uint16_t text_line(uint8_t line)
{
	int i = 0;
	int j = 0;
	while (i!=line)
	{
		if (j==sizeof(text)-1) return 0;
		if (text[j]=='\n') i++;
		j++;
	}
	return j--;
}

uint16_t text_line_len(uint8_t line)
{
	int i = 0;
	int line_offset = text_line(line);
	while (text[line_offset+i]!='\n')
	{
		i++;
	}
	return i--;
}

void init()
{
	sei();
	led_status = 1;
	DDRD = 255;
	DDRB = 255;
	PORTB = 0;
	PORTD = 0;
	
	text_clear();
	for (int i = 0; i<16; i++) rx_buf[i]=0;
}

void init_uart()
{
	UBRR0H=0;	
	UBRR0L=25;
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
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

void uart_send(uint8_t data) 
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
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

void d_clear()
{
	uart_send(4);
	uart_send(0);
	_delay_ms(50);
}

void d_print(uint8_t symb)
{
	uart_send(2);
	uart_send(symb);
}

void d_set(uint8_t pos)
{
	uart_send(3);
	uart_send(pos);
}

void d_println(uint8_t *line, uint8_t offset)
{
	d_set(offset);
	int i = 0;
	while((line[i]!='\n')&&(line[i]!='\0')&&(i<(16-(offset%16))))
	{
		d_print(line[i]);
		i++;
	}
}

uint16_t kb_read()
{
	int i = 0;
	uint16_t result;
	while (rx_buf[i]==0)
	{
		i+=2;
		if(i==16) i=0;
	}
	if (rx_buf[i]==6)
	{
		result = rx_buf[i+1];
	}
	if (rx_buf[i]==1)
	{
		result = 0x0100+rx_buf[i+1];
	}
	rx_buf[i] = 0;
	rx_buf[i+1] = 0;
	return result;
}

void led()
{
	uint16_t kb = 0;
	d_clear();
	d_println(led_on_off,0);
	while(1==1)
	{
		if (led_status == 0)
		{
			d_println(led_on,2);
			PORTB |= (1<<PB5);
		} else
		{
			d_println(led_off,2);
			PORTB &= ~(1<<PB5);
		}
		kb = kb_read();
		if(kb==0x0106)
		{
			led_status = 1;
		} else if (kb==0x0105)
		{
			led_status = 0;
		} else if (kb==0x0102)
		{
			return;
		}
	};
}

uint8_t check()
{
	for (int i = 0; i<sizeof(text);i++)
	{
		if ((text[i]!=' ')&&(text[i]!='.')&&(text[i]!=',')&&(text[i]!='>')&&(text[i]!='<')&&(text[i]!='+')&&(text[i]!='-')&&(text[i]!='[')&&(text[i]!=']')&&(text[i]!='\n'))
		{
			return 0;
		}
	}
	return 1;
}

void execute()
{
	d_clear();
	if (!check())
	{
		d_println(bf_error,0);
		kb_read();
		return;
	} 
	int ptr = 0;
	int	brc = 0;
	for (int i = 0;i<sizeof(text);i++)
	{
		if (text[i]=='>') ptr++;
		if (text[i]=='<') ptr--;
		if (text[i]=='+') cpu[ptr]++;
		if (text[i]=='-') cpu[ptr]--;
		if (text[i]=='.') 
		{
			d_print(cpu[ptr]);
		}
		if (text[i]==',') 
		{
			cpu[ptr]=kb_read();
		}
		if (text[i] == '[') 
		{
			if (!cpu[ptr]) 
			{
				brc++;
				while (brc) 
				{
					i++;
					if (text[i] == '[') brc++;
					if (text[i] == ']') brc--;
				}
			} else continue;
		} else if (text[i] == ']') 
		{
			if (!cpu[ptr])
			{
			 continue; 
			} else 
			{
				if (text[i] == ']') brc++;
				while (brc) 
				{
					i--;
					if (text[i] == '[') brc--;
					if (text[i] == ']') brc++;
				}
				i--;
			}
		}
	}
	d_clear();
	d_println(bf_complete,0);
	kb_read();
	for (int i = 0; i<256; i++) cpu[i]=0;	
}

void bf()
{
	uint16_t kb;
	uint16_t curs = 0;
	uint16_t line = 0;
	d_clear();
	d_println(text,0);
	d_set(0);
	while(1==1)
	{
		d_clear();
		if (curs<8)
		{
			d_println(text+text_line(line),0);
			if (text_line(line+1)!=0) d_println(text+text_line(line+1),16);
			d_set(curs);
		} else
		{
			d_println(text+text_line(line)+curs-8,0);
			if ((text_line(line+1)!=0)&&((curs-8)<text_line_len(line+1))) d_println(text+text_line(line+1)+curs-8,16);
			d_set(8);
		}
		
		kb = kb_read();
		if((kb>>8)==0)
		{
			if (text[sizeof(text)-2]=='\n')
			{
				text_add(kb,curs+text_line(line));
				curs++;
			}
		} else if (kb==0x0101)
		{
			if (curs>0)
			{
				curs--;
				text_rem(curs+text_line(line));
			}
		} else if (kb==0x0105)
		{
			if (curs>0)
			{
				curs--;
			}
		} else if (kb==0x0106)
		{
			if (text[curs+text_line(line)]!='\n')
			{
				curs++;
			}
		} else if (kb==0x0104)
		{
			if ((text[sizeof(text)-2]=='\n')||(text_line(line+1)!=0))
			{
				line++;
				curs = 0;
			}
		} else if (kb==0x0103)
		{
			if (line>0)
			{
				line--;
				curs = 0;
			}
		}
		else if (kb==0x0102)
		{
			execute();
		}
	};
}

void menu()
{
	uint8_t curs = 0;
	uint16_t kb = 0;
	menu_l:
	curs = 0;
	d_clear();
	d_println(basic_str,0);
	d_println(led_str,16);
	d_set(0);
	d_print('>');
	while (1==1)
	{
		kb = kb_read();
		if((kb==0x0104)&&(curs!=2))
		{
			d_clear();
			if (curs == 0) 
			{
				d_println(basic_str,0);
				d_println(led_str,16);
			} else
			{
				d_println(led_str,0);
				d_println(reboot_str,16);
			}
			d_set(16);
			d_print('>');
			curs++;
		} 
		else if((kb==0x0103)&&(curs!=0))
		{
			d_clear();
			if (curs == 1) 
			{
				d_println(basic_str,0);
				d_println(led_str,16);
			} else
			{
				d_println(led_str,0);
				d_println(reboot_str,16);
			}
			d_set(0);
			d_print('>');
			curs--;
		}
		else if((kb==0x0102))
		{
			if(curs==2)
			{
				return;
			}
			else if (curs==0)
			{
				bf();
				goto menu_l;
			}	else if (curs==1)
			{
				led();
				goto menu_l;
			}
		}
	}
}



void main() 
{
	while (1==1)
	{
		init();
		init_uart();
		d_clear();
		d_println(welcome_str,0);
		d_set(16);
		kb_read();
		d_clear();
		menu();
	}
}
