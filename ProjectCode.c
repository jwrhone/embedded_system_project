#include "mbed.h"
#include "i2c.h"

//DigitalOut CLK(p24); // clock is at p24
//DigitalOut CLR(p25); // clear is at p25
//DigitalOut DATA(p30); // data is at p30

//row scanning
DigitalIn R1(p17);
DigitalIn R2(p18);
DigitalIn R3(p16);
DigitalIn R4(p20);

DigitalOut C1(p5);
DigitalOut C2(p6);
DigitalOut C3(p7);
DigitalOut C4(p8);
DigitalOut LED(p9);


int rowScan(int colNum);
int rowPressed();
int getNum(int rowNum, int colNum);
int getOpNum();


#define decode_bcd(x) ((x >> 4) * 10 + (x & 0x0F)) 
#define encode_bcd(x) ((((x / 10) & 0x0F) << 4) + (x % 10)) 


static unsigned char wr_lcd_mode(char c, char mode);
unsigned char lcd_command(char c);
unsigned char lcd_data(char c);
void lcd_init(void);
void lcd_backlight(char on);
void setTime (int type);
void displayAlphabet();
int getFromPad(int len);
int getOneChar();
int getOpNum();
int rowPressed();
int rowScan(int colNum);
int normalMode();
int calcMode();
void fillArr(char word[], char ch, int size, int choice);
void clearArr();
int getSize(int result);
void usrLoop();

static char line[20];
static int arrcursor = 0;
static int prevCursor = 0;

// LCD Commands
// -------------------------------------------------------------------------
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
// flags for display entry mode
// -------------------------------------------------------------------------
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00
// flags for display on/off and cursor control
// -------------------------------------------------------------------------
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
// flags for display/cursor shift
// -------------------------------------------------------------------------
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00
// flags for function set
// -------------------------------------------------------------------------
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
///*********************************************
//Project : I2C to LCD Interface-Routine
//Port PCF8574 : 7  6  5  4  3  2  1 0
//              D7 D6 D5 D4 BL EN RW RS
//**********************************************/
#define PCF8574T 0x27

I2C i2c(p32, p31);// SDA, SCL//

void lcdLoad(char l[], char c, int size, int line, int position, int select){
	int location;
	switch(line){
		case 0: location = 0; break;
		case 1: location = 40; break;
		case 2: location = 20; break;
		case 3: location = 60; break;
	}
	
	for(int i = 0; i < location; i++){
		lcd_command(0x14); 
	}
	
	for(int j = 0; j < position; j++){
		lcd_command(0x14);
		location++;
	}
	
	if(select == 0){
		for(int i = 0; i < size - 1; i++){
			location++;
			lcd_data(l[i]);
		}
	}
	
	else{
		location++;
		lcd_data(c);
	}
	
	prevCursor = location;
	
	while (location != 0){
		location--;
		lcd_command(0x10); //wy 0x10
	}
		
}
void lcdPrev(char a[], char c, int size, int select){
    int location = prevCursor;
    for(int i = 0; i < location; i++){
        lcd_command(0x14);
    }

    if(select == 0){
        for(int i = 0; i < size-1; i++){
            location++;
            lcd_data(a[i]);
        }
    }else{
            location++;
            lcd_data(c);
    }

    prevCursor = location;

    while(location !=0){
            location--;
            lcd_command(0x10);
    }
	}
static unsigned char wr_lcd_mode(char c, char mode) {
 char ret = 1;
 char seq[5];
 static char backlight = 8;
 if (mode == 8) {
 backlight = (c != 0) ? 8 : 0;
 return 0;
 }
 mode |= backlight;
 seq[0] = mode; // EN=0, RW=0, RS=mode
 seq[1] = (c & 0xF0) | mode | 4; // EN=1, RW=0, RS=1
 seq[2] = seq[1] & ~4; // EN=0, RW=0, RS=1
 seq[3] = (c << 4) | mode | 4; // EN=1, RW=0, RS=1
 seq[4] = seq[3] & ~4; // EN=0, RW=0, RS=1
 i2c.start();
 i2c.write(PCF8574T << 1);
 uint8_t i;
 for (i = 0; i < 5; i++) {
 i2c.write(seq[i]);
 wait(0.002);
 }
 ret = 0;
 i2c.stop();
 if (!(mode & 1) && c <= 2)
 wait(0.2); // CLS and HOME
 return ret;
}
unsigned char lcd_command(char c) {
 wr_lcd_mode(c, 0);
}
unsigned char lcd_data(char c) {
 wr_lcd_mode(c, 1);
}
void lcd_init(void) {
 char i;

 // High-Nibble von Byte 8 = Display Control:
 // 1DCB**** D: Disp on/off; C: Cursor on/off B: blink on/off
 char init_sequenz[] = { 0x33, 0x32, 0x28, 0x0C, 0x06, 0x01 };
 wait(1); // Delay power-up
 for (i = 0; i < sizeof(init_sequenz); i++) {
 lcd_command(init_sequenz[i]);
 }
}
void lcd_backlight(char on) {
 wr_lcd_mode(on, 8);
}


void fillArr(char word[], char ch, int size, int choice){
	if(choice == 0){
		for(int i = 0; i < size - 1; i++){
			line[arrcursor] = word[i];
			arrcursor++;
		}
	}
	else{
		line[arrcursor] = ch;
		arrcursor++;
	}
}

void clearArr(){
	sprintf(line, "%d", 0);
	arrcursor = 0;
}


int main() {
	lcd_init();
	// setting the columns high (for row scanning 
	C1 = 1;
	C2 = 1;
	C3 = 1;
	C4 = 1;
	
	displayAlphabet();
	
	usrLoop();	
}

int calcModeToggel = 0;//normal display
void usrLoop(){
	 
	
	//start the real time clock
	i2c.start(); /* The following Enables the Oscillator */ 
	i2c.write(0xD0); /* address the part to write */ 
	i2c.write(0x00); /* position the address pointer to 0 */ 
	i2c.write(0x00); /* write 0 to the secs register, clear the CH bit */ 
	i2c.stop();
	
	//init the temp sensosr
	
	i2c.start();
	i2c.write(0x90);
	i2c.write(0xac); 
	i2c.write(2); 
	i2c.stop();
	setTime(0);
	setTime(1);
	int x = -1;
	while(1){
		if(calcModeToggel == 0){//normal display
			lcd_command(0x01);
			prevCursor = 0;
			x = normalMode();
			if(x == 1){
				calcModeToggel = 1;
				x = -1;
			}
		}
		if(calcModeToggel == 1){//calculator display
			lcd_command(0x01);
			prevCursor = 0;
			x = calcMode();
			if(x == 0){
				calcModeToggel = 0;
				x = -1;
			}
		}
	}
	
}

void displayAlphabet(){
	lcd_command(0x01);
	lcdLoad("ABCDEFGHIJKLM", ' ', sizeof("ABCDEFGHIJKLM"), 0, 0, 0);
	lcdLoad("NOPQRSTUVWXYZ", ' ', sizeof("ABCDEFGHIJKLM"), 1, 0, 0);
	wait(2);
	lcd_command(0x01);
	lcdLoad("ABCDEFGHIJKLM", ' ', sizeof("ABCDEFGHIJKLM"), 0, 0, 0);
	lcdLoad("NOPQRSTUVWXYZ", ' ', sizeof("ABCDEFGHIJKLM"), 1, 0, 0);
	lcdLoad("abcdefghijklm", ' ', sizeof("ABCDEFGHIJKLM"), 2, 0, 0);
	lcdLoad("nopqrstuvwxyz", ' ', sizeof("ABCDEFGHIJKLM"), 3, 0, 0);
	wait(2);
	lcd_command(0x01);
	lcdLoad("0123456789.", ' ', sizeof("0123456789."), 0, 0, 0);
	lcdLoad(".9876543210", ' ', sizeof("0123456789."), 1, 0, 0);
	wait(2);
	lcd_command(0x01);
	prevCursor = 0;
	
	
}

void setTime (int type){
int hour, min, ampm, month, date, day, year;
	if(type ==0){
		lcdLoad("set clock time", ' ', sizeof("set clock time"),2,0,0);
		wait(3);
		lcd_command(0x01);
		do{
			//getting clock time and hour 
			lcdLoad("Clock time Hour?", ' ', sizeof("Clock time Hour?"), 0, 0, 0);
			lcdLoad("Format: 04 = 4", ' ', sizeof("Format: 04 = 4"), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			hour = getFromPad(2);
			lcd_command(0x01);
		}while(hour >12 || hour <1);
		do{
			//getting clock time and minuite 
			lcdLoad("Clock time Min?", ' ', sizeof("Clock time Min?"), 0, 0, 0);
			lcdLoad("E to finalize", ' ', sizeof("E to finalize"), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			min = getFromPad(2);
			lcd_command(0x01);}
		while(min > 59);
		do{
				//getting the time am or pm
				lcdLoad("Clock time AM or PM?", ' ',sizeof("Clock time AM or PM?"),0,0,0);
				lcdLoad("(A for AM, B for PM)", ' ',sizeof("(A for AM, B for PM)"),1,0,0);
				lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
				ampm = getOneChar();
				lcd_command(0x01);
			}while (ampm!=10 && ampm != 11);
	
		do{
			//getting the month form the user 
			lcdLoad("Clock time Month?", ' ', sizeof("Clock time Month?"), 0, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			month = getFromPad(2);
			lcd_command(0x01);
		}while(month > 12 || month < 1);
		do{
			//getting the date from the user 
			lcdLoad("Clock time Date?", ' ', sizeof("Clock time Date?"), 0, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			date = getFromPad(2);
			lcd_command(0x01);
		}while(date > 31 || date < 1);
				
		do{
			//getting the day from the user
			lcdLoad("Clock time Day?", ' ', sizeof("Clock time Day?"), 0, 0, 0);
			lcdLoad("(Mon is 1, Sun is 7)", ' ', sizeof("(Mon is 1, Sun is 7)"), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			day = getFromPad(1);
			lcd_command(0x01);
		}while(day > 7 || day < 1);	
		do{
			//getting the year from the user 
			lcdLoad("Clock time Year?", ' ', sizeof("Clock time Year?"), 0, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			year = getFromPad(4);
			lcd_command(0x01);
		}while(year < 2000 || year > 2099);
			
		// defineing the ampm declesec
		if(ampm == 10){
			ampm = 0x40;
		}
		else if(ampm == 11){
			ampm = 0x60;
		}
		//write to the screen through i2c
// entering the clock data
		i2c.start();
		i2c.write(0xD0);
		i2c.write(0x00); // write register address 1st clock register
		i2c.write(encode_bcd(0)); // start at 0 seconds
		i2c.write(encode_bcd(min)); // the 57th minute
		i2c.write( ampm | encode_bcd(hour)); // the 11th hour pm
		i2c.write(encode_bcd(day)); // the 6th day of week (friday)
		i2c.write(encode_bcd(date)); // the 28th day of month
		i2c.write(encode_bcd(month)); // the 2nd month of the year (February) (Century bit 0)
		i2c.write(encode_bcd(year % 2000)); // the year is 2000 + 20 (a leap year)
		i2c.start();
		i2c.write(0xD0); 
		i2c.write(0x0e); // write register address control register
		i2c.write(0x20); // enable osc, bbsqi
		i2c.write(0); // clear the osf, alarm flags
		i2c.stop();
}
	else {
		//sett the alarm
		lcdLoad("Setting Alarm 1 time", ' ', sizeof("Setting Alarm 1 time"), 2, 0, 0);
		wait(5);
		lcd_command(0x01);
		do{
			//printf("alarm 1 time hour? ");
			lcdLoad("Alarm 1 time Hour?", ' ', sizeof("Alarm 1 time Hour?"), 0, 0, 0);
			lcdLoad("Format: 04 = 4", ' ', sizeof("Format: 04 = 4"), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			hour = getFromPad(2);
			lcd_command(0x01);
	}while (hour >12 || hour <1);
		do{
			//printf("alarm 1 time min? ");
			lcdLoad("Alarm 1 time Min?", ' ', sizeof("Alarm 1 time Min?"), 0, 0, 0);
			lcdLoad("E to finalize", ' ', sizeof("E to finalize"), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			min = getFromPad(2);
			lcd_command(0x01);
		}while(min > 59);
		do{
			//printf("alarm1 time am or pm? ");
				lcdLoad("Alarm 1 AM or PM?", ' ',sizeof("Alarm 1 AM or PM?"),0,0,0);
				lcdLoad("(A for AM, B for PM)", ' ',sizeof("(A for AM, B for PM)"),1,0,0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			ampm = getOneChar();
			lcd_command(0x01);
		}while(ampm != 10 && ampm != 11);
		do{
			//printf("alarm 1 time month? ");
			lcdLoad("Alarm 1 time Month? ", ' ', sizeof("Alarm 1 time Month? "), 0, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			month = getFromPad(2);
			lcd_command(0x01);
		}while(month > 12 || month < 1);
		do{
			//printf("alarm 1 time date? ");
			lcdLoad("Alarm 1 time Date? ", ' ', sizeof("Alarm 1 time Date? "), 0, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			date = getFromPad(2);
			lcd_command(0x01);
		}while(date > 31 || date < 1);
		do{
			//printf("alarm 1 time day? ");
			lcdLoad("Alarm 1 time Day?", ' ', sizeof("Alarm 1 time Day?"), 0, 0, 0);
			lcdLoad("(Mon is 1, Sun is 7)", ' ', sizeof("(Mon is 1, Sun is 7)"), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			day = getFromPad(1);
			lcd_command(0x01);
		}while(day > 7 || day < 1);
		do{
			//printf("alarm 1 time year? ");
			lcdLoad("Alarm 1 time Year?", ' ', sizeof("Alarm 1 time Year?"), 0, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 1, 0, 0);
			lcdLoad(" ", ' ', sizeof(" "), 3, 0, 0);
			year = getFromPad(4);
			lcd_command(0x01);
		}while(year < 2000 || year > 2099);
		
		if(ampm == 10){
			ampm = 0x40;
		}
		else if(ampm == 11){
			ampm = 0x60;
		}
		// writing the alarm data
		i2c.start();
		i2c.write(0xD0);
		i2c.write(0x07); // write register address 1st clock register
		i2c.write(encode_bcd(0)); // start at 0 seconds
		i2c.write(encode_bcd(min)); // the 57th minute
		i2c.write( ampm | encode_bcd(hour)); // the 11th hour pm
		i2c.write(0x00 | encode_bcd(date)); // the 28th day of month
		i2c.start();
		i2c.write(0xD0); 
		i2c.write(0x0e); // write register address control register
		i2c.write(0x20); // enable osc, bbsqi
		i2c.write(0); // clear the osf, alarm flags
		i2c.stop();
		
	}
		
		
}
static int alarmFired = 0;
int normalMode(){
	int sec, min, hrs, day, date,mon, year;
	int aMin, aHrs, aDate, aampm;
	int ampm;
	int alarmFlag = 0;
	 //read the values 
	i2c.start(); // start i2c 
	i2c.write(0xD0); // the address of the slave
	i2c.write(0x00); // the starting address to read (will increment automatically after each ACK)
	i2c.start(); // repeated i2c start
	i2c.write(0xD1); // we want to read from the slave
	sec = i2c.read(1); // read the seconds register, send ACK
	min = i2c.read(1); // read the minutes register, send ACK
	hrs = i2c.read(1); 						 // read the hour register, send ACK
	day = i2c.read(1); // read the day register, send ACK
	date = i2c.read(1); // read the Date register, send ACK
	mon = i2c.read(1); // read the month register, send ACK
	year = i2c.read(0); // read the year register, send NACK to end the communication
	i2c.stop(); // stop the i2c 
	
	i2c.start(); // start i2c 
	i2c.write(0xD0); // the address of the slave
	i2c.write(0x08); // the starting address to read (will increment automatically after each ACK)
	i2c.start(); // repeated i2c start
	i2c.write(0xD1); // we want to read from the slave
	aMin = i2c.read(1); // read the minutes register, send ACK
	aHrs = i2c.read(1); // read the hour register, send ACK
	aDate = i2c.read(0); // read the Date register, send NACK
	i2c.stop(); // stop the i2c 
	
	i2c.start(); // start i2c 
	i2c.write(0xD0); // the address of the slave
	i2c.write(0x0f); // the starting address to read (will increment automatically after each ACK)
	i2c.start(); // repeated i2c start
	i2c.write(0xD1); // we want to read from the slave
	alarmFlag = i2c.read(0); // read the alarm flag register, send NACK
	i2c.stop(); // stop the i2c 
	
	alarmFlag = alarmFlag & 0x01; // bit 0 is the alarm flag
	if (alarmFired == 0){
		alarmFired = alarmFlag;
	}
	
	// set the date valuse 
	ampm = (hrs & 0x20) >> 5;			 // get the ampm bit
	sec = decode_bcd(sec);
	min = decode_bcd(min);
	hrs = decode_bcd((hrs & 0x1F));
	day = decode_bcd(day);
	date = decode_bcd(date);
	mon = decode_bcd(mon);
	year = decode_bcd(year);
	
	aampm = (aHrs & 0x20) >> 5;	
	aMin = decode_bcd(aMin);
	aHrs = decode_bcd((aHrs & 0x1F));
	aDate = decode_bcd(aDate);
	
		int temperature;
	
	int upperByte;
	int lowerByte;
	
	i2c.start();
	i2c.write(0x90);
	i2c.write(0x51);
	i2c.stop();
	
	i2c.start();
	i2c.write(0x90);
	i2c.write(0xaa);

	i2c.start(); 
	i2c.write(0x91); 
	upperByte = i2c.read(1); // send an ACK after read
	lowerByte = i2c.read(0); // send a NACK after read
	temperature = (upperByte << 8) + lowerByte;
	i2c.stop();
	
	//tempature values 
	float tempF;
	float tempC;
	
	tempC = temperature >> 4;
	if (tempC > 0x7FF){ // if negative
		tempC = -1 * ( ((int) (2 << 12) - tempC) * 0.0625) ; // 2s comp convert
	}else{
		tempC = tempC * 0.0625;
	}
	
	tempF = (tempC * 1.8) + 32;
	
	
	
	if (alarmFlag == 1){
		lcdLoad("Alarm 1 has expired", ' ', sizeof("Alarm 1 has expired"), 0,0,0);
		LED = 1;
	}
	
	char tmp[3];
	int tsize = sizeof(tmp);
	sprintf(tmp, "%d", hrs);
	if(hrs < 10){
		fillArr(tmp, '0', tsize, 1);
		fillArr(tmp, ' ', tsize-1, 0);
	}
	else{
		fillArr(tmp, ' ', tsize, 0);
	}
	fillArr(" ", ':', 1, 1);
	
	sprintf(tmp, "%d", min);
	
	if(min < 10){
		fillArr(tmp, '0', tsize, 1);
		fillArr(tmp, ' ', tsize-1, 0);
	}
	else{
		fillArr(tmp, ' ', tsize, 0);
	}
	
	
	if(ampm == 1){
		fillArr("pm ", ' ', sizeof("pm "), 0);
	}
	else{
		fillArr("am ", ' ', sizeof("am "), 0);
	}
	switch(mon){
		case 1: fillArr("Jan ", ' ', sizeof("Jan "), 0);break;
		case 2: fillArr("Feb ", ' ', sizeof("Feb "), 0);break;
		case 3: fillArr("Mar ", ' ', sizeof("Mar "), 0);break;
		case 4: fillArr("Apr ", ' ', sizeof("Apr "), 0);break;
		case 5: fillArr("May ", ' ', sizeof("May "), 0);break;
		case 6: fillArr("Jun ", ' ', sizeof("Jun "), 0);break;
		case 7: fillArr("Jul ", ' ', sizeof("Jul "), 0);break;
		case 8: fillArr("Aug ", ' ', sizeof("Aug "), 0);break;
		case 9: fillArr("Sep ", ' ', sizeof("Sep "), 0);break;
		case 10: fillArr("Oct ", ' ', sizeof("Oct "), 0);break;
		case 11: fillArr("Nov ", ' ', sizeof("Nov "), 0);break;
		case 12: fillArr("Dec ", ' ', sizeof("Dec "), 0);break;
	}
	sprintf(tmp, "%d", date);
	
	if(date < 10){
		fillArr(tmp, '0', tsize, 1);
		fillArr(tmp, ' ', tsize-1, 0);
	}
	else{
		fillArr(tmp, ' ', tsize, 0);
	}
	
	fillArr(", ", ' ', sizeof(", "), 0);
	
	char yrs[5];
	sprintf(yrs, "%d", 2000+year);
	int ysize = sizeof(yrs);
	fillArr(yrs, ' ', ysize, 0);
	
	lcdLoad(line, ' ', sizeof(line) + 1, 1, 0, 0);
	if (alarmFlag == 1){
		LED = 0;
	}
	clearArr();
	char tc[5];
	int size = sizeof(tc);
	sprintf(tc, "%f", tempC);
	
	char tf[5];
	sprintf(tf, "%f", tempF);
	
	fillArr("Temp: ", ' ', sizeof("Temp: "), 0);
	fillArr(tc, ' ', size, 0);
	fillArr(" ",0xDF, 1, 1);
	fillArr(" ",'C', 1, 1);
	fillArr(" ",'(', 1, 1);
	fillArr(tf, ' ', size, 0);
	fillArr(" ",0xDF, 1, 1);
	fillArr(" ",'F', 1, 1);
	fillArr(" ",')', 1, 1);
	lcdLoad(line, ' ', sizeof(line) + 1, 2, 0, 0);
	if (alarmFlag == 1){
		LED = 1;
		wait(0.5);
		LED = 0;
	}
	clearArr();
	int chr;
	int colNum =1;
	float counter = 0;

	
	while(counter < 1.25){
		chr = rowScan(colNum);
		if(chr != 0){
			chr = getNum(chr, colNum);
			if(chr == 15){
				if(alarmFlag == 1){
					i2c.start();
					i2c.write(0xD0);
					i2c.write(0x0f);
					i2c.write(0x00);
					i2c.stop();
					return 0;
				}
				return 1;
			}
			else if(chr == 14 && alarmFired == 0){
				//display the alarm settings
				lcd_command(0x01); // clear the screen
				lcdLoad("Alarm1 Settings:", ' ', sizeof("Alarm1 Settings:"), 0,0,0);
				char tmp[3];
				int tsize = sizeof(tmp);
				sprintf(tmp, "%d", aHrs);
				
				if(aHrs < 10){
					fillArr(tmp, '0', tsize, 1);
					fillArr(tmp, ' ', tsize-1, 0);
				}
				else{
					fillArr(tmp, ' ', tsize, 0);
				}
				fillArr(" ", ':', 1, 1);
				
				sprintf(tmp, "%d", aMin);
				if(aMin < 10){
					fillArr(tmp, '0', tsize, 1);
					fillArr(tmp, ' ', tsize-1, 0);
				}
				else{
					fillArr(tmp, ' ', tsize, 0);
				}
				
				if(aampm == 1){
					fillArr("pm ", ' ', sizeof("pm "), 0);
				}
				else{
					fillArr("am ", ' ', sizeof("am "), 0);
				}
				
				
				switch(mon){
					case 1: fillArr("Jan ", ' ', sizeof("Jan "), 0);break;
					case 2: fillArr("Feb ", ' ', sizeof("Feb "), 0);break;
					case 3: fillArr("Mar ", ' ', sizeof("Mar "), 0);break;
					case 4: fillArr("Apr ", ' ', sizeof("Apr "), 0);break;
					case 5: fillArr("May ", ' ', sizeof("May "), 0);break;
					case 6: fillArr("Jun ", ' ', sizeof("Jun "), 0);break;
					case 7: fillArr("Jul ", ' ', sizeof("Jul "), 0);break;
					case 8: fillArr("Aug ", ' ', sizeof("Aug "), 0);break;
					case 9: fillArr("Sep ", ' ', sizeof("Sep "), 0);break;
					case 10: fillArr("Oct ", ' ', sizeof("Oct "), 0);break;
					case 11: fillArr("Nov ", ' ', sizeof("Nov "), 0);break;
					case 12: fillArr("Dec ", ' ', sizeof("Dec "), 0);break;
				}
				
				sprintf(tmp, "%d", aDate);
				if(aDate < 10){
					fillArr(tmp, '0', tsize, 1);
					fillArr(tmp, ':', tsize-1, 0);
				}
				else{
					fillArr(tmp, ' ', tsize, 0);
				}
				
				fillArr(", ", ' ', sizeof(", "), 0);
				
				char yrs[5];
				sprintf(yrs, "%d", 2000+year);
				int ysize = sizeof(yrs);
				fillArr(yrs, ' ', ysize, 0);
				
				lcdLoad(line, ' ', sizeof(line) + 1, 1, 0, 0);
				
				
				clearArr();
				
				do{
					chr = rowScan(colNum);
					if(chr != 0){
						chr = getNum(chr, colNum);
					}
					if(colNum == 4){ 
						colNum = 1;
					}
					else{
						colNum++;
					}
					wait(0.03);
				}while(chr != 14 && chr != 15);
				
				switch(chr){
					case 14: return 0;
					case 15: return 1;
				}
			}
		}
	if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
		counter += 0.03;
	}
	//if alarm flag is set write it to the lcd and blink led and display current time and date 
	return 0;
	
}
	
	int getOneChar(){
	int count = 0;
	int first = 0;
	int num = 0;
	int colNum = 1;
	char chars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	while(count < 1){
		first = rowScan(colNum);
		if(first != 0){
			first = getNum(first, colNum);
			count++;
			lcdPrev(" ", chars[first], 1, 1);
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
	}
	
	num = first;
	return num;
}
	
int calcMode(){
	int op1;
	int op2;
	int opNum;
	int result = 0;
  int colNum = 1;
	
	op1 = getFromPad(3);
	result = op1;
	
	opNum = getOpNum();
	
	
	while(opNum != 14 && opNum != 15){
		switch(opNum){
			case 10: lcdPrev(" ", '+', 1, 1 );break;
			case 11: lcdPrev(" ", '-', 1, 1 );break;
			case 12: lcdPrev(" ", '*', 1, 1 );break;
			case 13: lcdPrev(" ", '/', 1, 1 );break;
		}

		op2 = getFromPad(3);

		switch(opNum){
				case 10: result = result + op2; break;// add
				case 11: result = result - op2; break;// subtract
				case 12: result = result * op2; break;// multiply
				case 13: result = result / op2; break;// divide
		}

		opNum = getOpNum();
	}
	
	if (opNum == 15){
		// we'll force it out of calc mode
		// into normal mode
			//clearScreen();
		lcd_command(0x01);
		calcModeToggel = 0;
		return 0;
	}
	else if(opNum == 14){
			//displayResult(result);
		char tst[7];
		sprintf(tst, "=%d", result);
		lcdLoad(tst, ' ', getSize(result), 3,0,0);
		wait(0.1);
		while(rowScan(colNum) == 0){
				if(colNum == 4){
						colNum = 1;
				}
				else{
						colNum++;
				}
				wait(0.05);
		}
		//clearScreen();
		lcd_command(0x01);
		calcModeToggel = 1;
		return 1;
	}
		
}
int getSize(int result){
	int counter = 6;
	int outcome = 0;
	int startingDiv = 100000;
	if(result == 0){
		return 3;
	}
	
	while(outcome == 0){
		outcome = result/startingDiv;
		
		if(outcome != 0){
			if (result < 0){
				return counter + 3;
			}
			else{
				return counter + 2;
			}
		}
		else{
			counter--;
			startingDiv /= 10;
		}
			
		
	}
}

int getFromPad(int len){ //len = 0,1,2,3,4 depending on expected input length
		int count = 0;
		int first = 0;
		int second = 0;
		int third = 0;
		int fourth = 0;
		int num = 0;
		int colNum = 1;
	
	while(count < len){
		if(count == 0){
			first = rowScan(colNum);
			if(first != 0){
				first = getNum(first, colNum);
				if(first < 10){
					count++;
					//printf("first");
					char c = first + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		else if(count == 1){
			second = rowScan(colNum);
			if(second != 0){
				second = getNum(second, colNum);
				if(second < 10){
					count++;
					//printf("second");
					char c = second + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		else if(count == 2){
			third = rowScan(colNum);
			if(third != 0){
				third = getNum(third, colNum);
				if(third < 10){
					count++;
					//printf("third");
					char c = third + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		else if(count == 3){
			fourth = rowScan(colNum);
			if(fourth != 0){
				fourth = getNum(fourth, colNum);
				if(fourth < 10){
					count++;
					//printf("fourth");
					char c = fourth + '0';
					lcdPrev(" ", c, 1, 1);
				}
			}
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
		
	}
	switch(len){
		case 1: num = first; break;
		case 2: num = first*10 + second; break;
		case 3: num = first*100 + second*10 + third; break;
		case 4: num = first*1000 + second*100 + third*10 + fourth; break;
	}
	return num;
}
	
int getNum(int rowNum, int colNum){
		int numOrder[4][4] = {{1,2,3,10}, 
											 {4,5,6,11}, 
											 {7,8,9,12}, 
											 {0,15,14,13}};
		return numOrder[rowNum-1][colNum-1];
}
int rowScan(int colNum){
	int rowNum = 0;
	if(colNum == 1){
		C1 = 0;
		C2 = 1;
		C3 = 1;
		C4 = 1;
	}
	else if(colNum == 2){
		C1 = 1;
		C2 = 0;
		C3 = 1;
		C4 = 1;
	}
	else if(colNum == 3){
		C1 = 1;
		C2 = 1;
		C3 = 0;
		C4 = 1;
	}
	else if(colNum == 4){
		C1 = 1;
		C2 = 1;
		C3 = 1;
		C4 = 0;
	}
	
	rowNum = rowPressed();
	
	C1 = 1;
	C2 = 1;
	C3 = 1;
	C4 = 1;
	return rowNum;
}

int rowPressed(){
	int num = 0;
	if(R1 == 0){
		num = 1;
	}
	else if(R2 == 0){
		num = 2;
	}
	else if(R3 == 0){
		num = 3;
	}
	else if(R4 == 0){
		num = 4;
	}
	return num;
}



int getOpNum(){
	int op = 0;
	int colNum = 1;
	while(op < 10){
		op = rowScan(colNum);
		if(op != 0){
			op = getNum(op, colNum);
		}
		
		if(colNum == 4){ 
			colNum = 1;
		}
		else{
			colNum++;
		}
		wait(0.03);
	}
	return op;
}
