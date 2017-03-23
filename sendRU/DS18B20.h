#ifndef DS18B20_H
#define DS18B20_h

#define DQ P0_0
#define ERROR 0xf0
//#define SUCCESS 0xff
unsigned char init_DS18B20(void);
void write_byte(unsigned char dat);
unsigned char read_byte(void);
unsigned char get_temperature(void);
unsigned char read_temperature(void);

#endif
