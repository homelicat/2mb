#define F_CPU 16000000UL  
#include <avr/io.h>        
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t rx_buf[16];
uint8_t rx_last = 0;
uint8_t led_status = 1;

char welcome_str[] = "2MB 4.2 Hmundik";
char busel_str[] = " Busel Int.";
char reboot_str[] = " Reboot";
char led_str[] = " Led";
char led_on_off[] = "ON   OFF";
char led_on[] = "<==";
char led_off[] = "==>";
char bf_error_hex[] = "Err: bad hex";
char bf_error_label[] = "Err: no label";
char bf_error_com[] = "Err: bad command";
char bf_complete[] = "Complete";


char text[1024];
uint8_t mem[16] = {0};
uint8_t reg_mem = 0;
uint8_t reg_arga = 0;
uint8_t reg_argb = 0;
uint8_t reg_cond = 0;


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

uint8_t is_hex(char x1,char x2)
{
	if((((x1>='0')&&(x1<='9'))||((x1>='a')&&(x1<='f'))||((x1>='A')&&(x1<='F')))&&(((x2>='0')&&(x2<='9'))||((x2>='a')&&(x2<='f'))||((x2>='A')&&(x2<='F')))) 
	{
		return 1;
	}
	return 0;
}

uint8_t hex(char x1, char x2)
{
	uint8_t res = 0;
	if (x1=='1') res+=0x10;
	if (x1=='2') res+=0x20;
	if (x1=='3') res+=0x30;
	if (x1=='4') res+=0x40;
	if (x1=='5') res+=0x50;
	if (x1=='6') res+=0x60;
	if (x1=='7') res+=0x70;
	if (x1=='8') res+=0x80;
	if (x1=='9') res+=0x90;
	if (x1=='a') res+=0xa0;
	if (x1=='b') res+=0xb0;
	if (x1=='c') res+=0xc0;
	if (x1=='d') res+=0xd0;
	if (x1=='e') res+=0xe0;
	if (x1=='f') res+=0xf0;
	if (x1=='A') res+=0xa0;
	if (x1=='B') res+=0xb0;
	if (x1=='C') res+=0xc0;
	if (x1=='D') res+=0xd0;
	if (x1=='E') res+=0xe0;
	if (x1=='F') res+=0xf0;
	if (x2=='1') res+=0x1;
	if (x2=='2') res+=0x2;
	if (x2=='3') res+=0x3;
	if (x2=='4') res+=0x4;
	if (x2=='5') res+=0x5;
	if (x2=='6') res+=0x6;
	if (x2=='7') res+=0x7;
	if (x2=='8') res+=0x8;
	if (x2=='9') res+=0x9;
	if (x2=='a') res+=0xa;
	if (x2=='b') res+=0xb;
	if (x2=='c') res+=0xc;
	if (x2=='d') res+=0xd;
	if (x2=='e') res+=0xe;
	if (x2=='f') res+=0xf;
	if (x2=='A') res+=0xa;
	if (x2=='B') res+=0xb;
	if (x2=='C') res+=0xc;
	if (x2=='D') res+=0xd;
	if (x2=='E') res+=0xe;
	if (x2=='F') res+=0xf;
	return res;
}

void execute()
{
	d_clear();
	for (int i = 0;i<sizeof(text);i++)
	{
		if ((text[i]==' ')||(text[i]=='\n'))
		{
			
		} else
		if ((text[i]=='m')&&(text[i+1]=='e')&&(text[i+2]=='m')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			reg_mem=hex(text[i+4+j],text[i+4+j+1]);
			i+=j+4+2;
		} else
		if ((text[i]=='s')&&(text[i+1]=='t')&&(text[i+2]=='r')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			mem[reg_mem]=hex(text[i+4+j],text[i+4+j+1]);
			i+=j+4+2;
		} else
		if ((text[i]=='o')&&(text[i+1]=='u')&&(text[i+2]=='t')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			d_set(mem[hex(text[i+4+j],text[i+4+j+1])]);
			d_print(mem[reg_mem]);
			i+=j+4+2;
		} else
		if ((text[i]=='m')&&(text[i+1]=='o')&&(text[i+2]=='v')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			mem[reg_mem]=mem[hex(text[i+4+j],text[i+4+j+1])];
			i+=j+4+2;
		} else
		if ((text[i]=='i')&&(text[i+1]=='n')&&((text[i+2]==' ')||(text[i+2]=='\n')||(text[i+2]=='\0')))
		{
			uint8_t key = kb_read();
			mem[reg_mem]=key;
			i+=2;
		} else
		if ((text[i]=='a')&&(text[i+1]=='d')&&(text[i+2]=='d')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			mem[reg_mem]+=hex(text[i+4+j],text[i+4+j+1]);
			i+=j+4+2;
		} else
		if ((text[i]=='s')&&(text[i+1]=='u')&&(text[i+2]=='b')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			mem[reg_mem]-=hex(text[i+4+j],text[i+4+j+1]);
			i+=j+4+2;
		} else
		if ((text[i]=='j')&&(text[i+1]=='m')&&(text[i+2]=='p')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			int j = 0;
			while ((text[i+4+j]==' ')||(text[i+4+j]=='\n')) j++;
			if ((!is_hex(text[i+4+j],text[i+4+j+1]))||((text[i+4+j+2]!=' ')&&(text[i+4+j+2]!='\n')&&(text[i+4+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			uint8_t point = mem[hex(text[i+4+j],text[i+4+j+1])];
			uint16_t addr = 0;
			for (j = 0;j<sizeof(text);j++)
			{
				if((text[j]=='#')&&(point==hex(text[j+1],text[j+2])))
				{
					addr=j+3;
				}
			}
			if(addr==0)
			{
				d_println(bf_error_label,0);
				kb_read();
				return;
			}
			i=addr;
		} else
		if ((text[i]=='a')&&(text[i+1]=='r')&&(text[i+2]=='g')&&(text[i+3]=='a')&&((text[i+4]==' ')||(text[i+4]=='\n')||(text[i+4]=='\0')))
		{
			int j = 0;
			while ((text[i+5+j]==' ')||(text[i+5+j]=='\n')) j++;
			if ((!is_hex(text[i+5+j],text[i+5+j+1]))||((text[i+5+j+2]!=' ')&&(text[i+5+j+2]!='\n')&&(text[i+5+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			reg_arga = mem[hex(text[i+5+j],text[i+5+j+1])];
			i+=j+5+2;
		} else
		if ((text[i]=='a')&&(text[i+1]=='r')&&(text[i+2]=='g')&&(text[i+3]=='b')&&((text[i+4]==' ')||(text[i+4]=='\n')||(text[i+4]=='\0')))
		{
			int j = 0;
			while ((text[i+5+j]==' ')||(text[i+5+j]=='\n')) j++;
			if ((!is_hex(text[i+5+j],text[i+5+j+1]))||((text[i+5+j+2]!=' ')&&(text[i+5+j+2]!='\n')&&(text[i+5+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			reg_argb = mem[hex(text[i+5+j],text[i+5+j+1])];
			i+=j+5+2;
		} else
		if ((text[i]=='c')&&(text[i+1]=='o')&&(text[i+2]=='n')&&(text[i+3]=='d')&&((text[i+4]==' ')||(text[i+4]=='\n')||(text[i+4]=='\0')))
		{
			int j = 0;
			while ((text[i+5+j]==' ')||(text[i+5+j]=='\n')) j++;
			if ((!is_hex(text[i+5+j],text[i+5+j+1]))||((text[i+5+j+2]!=' ')&&(text[i+5+j+2]!='\n')&&(text[i+5+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			reg_cond = mem[hex(text[i+4+j],text[i+4+j+1])];
			i+=j+5+2;
		} else
		if ((text[i]=='j')&&(text[i+1]=='c')&&((text[i+2]==' ')||(text[i+2]=='\n')||(text[i+2]=='\0')))
		{
			int j = 0;
			while ((text[i+3+j]==' ')||(text[i+3+j]=='\n')) j++;
			if ((!is_hex(text[i+3+j],text[i+3+j+1]))||((text[i+3+j+2]!=' ')&&(text[i+3+j+2]!='\n')&&(text[i+3+j+2]!='\0')))
			{
				d_println(bf_error_hex,0);
				kb_read();
				return;
			}
			uint8_t point = mem[hex(text[i+4+j],text[i+4+j+1])];
			uint16_t addr = 0;
			i+=3+j+2;
			for (j = 0;j<sizeof(text);j++)
			{
				if((text[j]=='#')&&(point==hex(text[j+1],text[j+2])))
				{
					addr=j+3;
				}
			}
			if(addr==0)
			{
				d_println(bf_error_label,0);
				kb_read();
				return;
			}
			if (reg_cond==1)
			{
				if(!(reg_arga==reg_argb))
				{
					addr=i;
				}
			} else
			if (reg_cond==2)
			{
				if(!(reg_arga!=reg_argb))
				{
					addr=i;
				}
			} else
			if (reg_cond==3)
			{
				if(!(reg_arga>reg_argb))
				{
					addr=i;
				}
			} else
			if (reg_cond==4)
			{
				if(!(reg_arga<reg_argb))
				{
					addr=i;
				}
			} else
			if (reg_cond==5)
			{
				if(!(reg_arga>=reg_argb))
				{
					addr=i;
				}
			} else
			if (reg_cond==6)
			{
				if(!(reg_arga<=reg_argb))
				{
					addr=i;
				}
			}
			i=addr;
		} else
		if ((text[i]=='#')&&(is_hex(text[i+1],text[i+2]))&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			i+=3;
		} else
		if ((text[i]=='s')&&(text[i+1]=='t')&&(text[i+2]=='p')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			i=1023;
		} else
		if ((text[i]=='n')&&(text[i+1]=='o')&&(text[i+2]=='p')&&((text[i+3]==' ')||(text[i+3]=='\n')||(text[i+3]=='\0')))
		{
			i+=4;
		} else
		{
			d_println(bf_error_com,0);
			kb_read();
			return;
		}
	}
	kb_read();
	d_clear();
	d_println(bf_complete,0);
	kb_read();
	for (int i = 0; i<16; i++) mem[i]=0;	
}

void busel()
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
	d_println(busel_str,0);
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
				d_println(busel_str,0);
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
				d_println(busel_str,0);
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
				busel();
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
