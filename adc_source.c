#include <stdio.h>
#include <stdlib.h>
#include "xc.h"
#include <p24FJ64GA002.h>

#include "string.h"

// CW1: FLASH CONFIGURATION WORD 1 (see PIC24 Family Reference Manual 24.1)
#pragma config ICS = PGx1          // Comm Channel Select (Emulator EMUC1/EMUD1 pins are shared with PGC1/PGD1)
#pragma config FWDTEN = OFF        // Watchdog Timer Enable (Watchdog Timer is disabled)
#pragma config GWRP = OFF          // General Code Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF           // General Code Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = OFF        // JTAG Port Enable (JTAG port is disabled)


// CW2: FLASH CONFIGURATION WORD 2 (see PIC24 Family Reference Manual 24.1)
#pragma config I2C1SEL = PRI       // I2C1 Pin Location Select (Use default SCL1/SDA1 pins)
#pragma config IOL1WAY = OFF       // IOLOCK Protection (IOLOCK may be changed via unlocking seq)
#pragma config OSCIOFNC = ON       // Primary Oscillator I/O Function (CLKO/RC15 functions as I/O pin)
#pragma config FCKSM = CSECME      // Clock Switching and Monitor (Clock switching is enabled, 
                                       // Fail-Safe Clock Monitor is enabled)
#pragma config FNOSC = FRCPLL      // Oscillator Select (Fast RC Oscillator with PLL module (FRCPLL))

//Global Variables
volatile unsigned int Buffer[512];
volatile unsigned int myAverage = 0;
volatile unsigned int BuffSize = 512;
volatile unsigned int buffElement = 0;

//Initialize the Buffer
void initBuffer(int Val)            //set all bugger values to zero
{
    int i = 0;
    for(i=0;i<(BuffSize-1);i++)
    {
        Buffer[i] = 0;              //Update each buffer element to 0    
    }
}

//Calculate Average based on most recent buffer values
int getAvg(int Buffer[])    
{
    int total = 0, i = 10, avg;     //New variables
    
    for( i=0; i < 16; i++)
    {
       total = total + Buffer[i];   //Calculating total of all elements added
    }
    
    avg = total/(16);               //Calculating average of 16 elements
    return(avg);                    //Return the average value
}

//Update buffer with new values in incrementing elements 
void putVal(int newValue)  //add a new value to the buffer
{
    //if buffer is not full
    if(buffElement < 16)
    {
        Buffer[buffElement] = newValue;     //insets a new value into buffer
        buffElement++;                      //moves to the next element to accept a variable next
    }
    
    //if buffer is full
    else
    {
        buffElement = 0;                    //resets buffer to start at element 0
        Buffer[buffElement] = newValue;     //insets a new value into buffer
        buffElement++;                      //moves to the next element to accept a variable next
    }  
}

//Millisecond Delay Function
void delayms(int ms)
{
    int i = 0;
    while(i<ms)
    {
        wait_1ms();  
        i++;
    }
}

//Microsecond delay function
void delayus(int us)
{
    int i = 0;
    while(i<us)
    {
        wait_1us();  
        i++;
    }
}

//Function that displays an entire string
void lcd_printChar(char myChar)
{
    I2C2CONbits.SEN = 1;            //Begin (S)tart Sequence
    while(I2C2CONbits.SEN ==1);     //Wait for start bit to clear
    IFS3bits.MI2C2IF = 0;           //Clear interrupt flag
    
    I2C2TRN = 0b01111100;           //8-bits consisting of the salve address and the R/nW bit
    while(IFS3bits.MI2C2IF == 0);   //Software reset after the MI2C2IF has been triggered
    IFS3bits.MI2C2IF = 0;           //Clear the Interrupt Flag
    
    I2C2TRN = 0b01000000;           //8-bit control byte, C0 = 0 is "last byte", RS = 1 
    while(IFS3bits.MI2C2IF == 0);   //Software reset after the MI2C2IF has been triggered
    IFS3bits.MI2C2IF = 0;           //Clear the Interrupt Flag
    
    I2C2TRN = myChar;               //8-bit data byte
    while(IFS3bits.MI2C2IF == 0);   //Software reset after the MI2C2IF has been triggered
    IFS3bits.MI2C2IF = 0;           //Clear the Interrupt Flag
    
    I2C2CONbits.PEN = 1;            //Begin Sto(P) Sequence
    while(I2C2CONbits.PEN == 1);    //Wait for the End of the Function bits
}

//Function to tell where incoming string of character/data commands will start on LCD
void lcd_setCursor(int x, int y)
{
    int DDRAM = (y*0x40) + x;       //p.13 background lab info
    DDRAM |= 0b10000000;            //0x40 is 0b01000000 and we also need 0b10000000 for setting DDRAM address
    lcd_cmd(DDRAM);                 //Setting the address to write characters to
}

//Function to initialize the LCD
void lcd_init(void)
{
    delayms(50);
    lcd_cmd(0b00111000);    //function set
    delayus(35);
    lcd_cmd(0b00111001);    //function set
    delayus(35);
    lcd_cmd(0b00010100);    //Internal OSC frequency 
    delayus(35);
    lcd_cmd(0b01110000);    //Contrast set
    delayus(35);
    lcd_cmd(0b01010110);    //Power/ICON/Contrast Control
    delayus(35);
    lcd_cmd(0b01101100);    //Follower Control
    delayms(210);           //Wait for power to stabilize
    lcd_cmd(0b00111000);    //Function set
    delayus(35);
    lcd_cmd(0b00001100);    //Display ON/OF Control
    delayus(35);
    lcd_cmd(0b00000001);    //Clear Display
    delayms(2); 
    lcd_cmd(0b00000010);    //Return Cursor Home  
}

//This function organizes the data packets sent and is used to modify control settings on LCD
void lcd_cmd(char command)
{
    I2C2CONbits.SEN = 1;            //Begin (S)tart Sequence
    while(I2C2CONbits.SEN ==1);     //Wait for start bit to clear
    IFS3bits.MI2C2IF = 0;           //Clear interrupt flag
    
    I2C2TRN = 0b01111100;           //8-bits consisting of the salve address and the R/nW bit
    while(IFS3bits.MI2C2IF == 0);   //Software reset after the MI2C2IF has been triggered
    IFS3bits.MI2C2IF = 0;           //Clear the Interrupt Flag
    
    I2C2TRN = 0b00000000;           //8-bit control byte
    while(IFS3bits.MI2C2IF == 0);   //Software reset after the MI2C2IF has been triggered
    IFS3bits.MI2C2IF = 0;           //Clear the Interrupt Flag
    
    I2C2TRN = command;              //8-bit data byte
    while(IFS3bits.MI2C2IF == 0);   //Software reset after the MI2C2IF has been triggered
    IFS3bits.MI2C2IF = 0;           //Clear the Interrupt Flag
    
    I2C2CONbits.PEN = 1;            //Begin Sto(P) Sequence
    while(I2C2CONbits.PEN == 1);    //Wait for the end of the function bits  
}

//This function sets the Baud rate for the I2C register
void setBaudRG(void)
{
    I2C2CONbits.I2CEN = 0;      //Disable I2C2 peripheral 
    IFS3bits.MI2C2IF = 0;       //Reset the interrupt flag
    I2C2BRG = 0x9D;
    I2C2CONbits.I2CEN = 1;  
}

//This function sets up the program, initializes components and registers
void setup(void)
{
    CLKDIVbits.RCDIV = 0;       //sets clock freq. to 32MHz
    AD1PCFG = 0x9FFE;           //Set all pins digital except for AN0
    TRISB = TRISB & 0b0011;     //Set outputs 
    TRISA = 0x0001;             //Set AN0 as analog input
    setBaudRG();                //Setup Baud Rate
    lcd_init();                 //Initialize the LCD
    
    //Timer 2 / Display Timer setup
    T2CON = 0;                  //Clear timer 2 control register
    TMR2 = 0;                   //Clear the timer 2
    PR2 = 24999;                //Cycles to create 100ms 
    T2CONbits.TCKPS = 0b10;     //PRE = 64 to make sure PR2 < 34999
    IEC0bits.T2IE = 1;          //Enable the TMR2 Interrupt
    IPC1bits.T2IP = 0b001;      //Make this timer interrupt the first priority
    IFS0bits.T2IF = 0;          //Clear TMR2 interrupt flag
    T2CONbits.TON = 1;          //Start TMR2 for LCD display
    
    //Timer 3 / Sampling Timer setup
    T3CON = 0;              //Clear timer 3 control register
    TMR3 = 0;               //Clear timer 3
    PR3 = 15624;            //Cycles to create 62.5ms period for sampling
    T3CONbits.TCKPS = 0b10; //PRE = 64 to make sure PR3 < 34999
    IEC0bits.T3IE = 0;      //We'll tie the ADC ISR to this timer instead of using this timer's ISR
    IFS0bits.T3IF = 0;      //Clear Interrupt flag
    T3CONbits.TON = 1;      //Start TMR3 for sampling
    
    
    //Setting up A/D
    AD1CON1 = 0x0000;           //Clear the A/D control register
    AD1CON1bits.ADON = 0;       //bit 15 - Engages A/D mode as "Off" 
    AD1CON1bits.ADSIDL = 0;     //bit 13 - continues operation in idle mode
    //bits 14,12-10, and 4-3 are read as 0 and don't have functions
    AD1CON1bits.FORM = 0b00;
    AD1CON1bits.SSRC = 0b010;   //Selecting TMR3 as trigger source
    AD1CON1bits.ASAM = 1;       //Sampling begins immediately after last conversion
    
    AD1CON2 = 0;                //Clearing the AD control 2 register
    
    AD1CON3 = 0;                //Clearing the AD control 3 register
    AD1CON3bits.SAMC = 0b00001; //Auto Sample bits get 1 Tad
    AD1CON3bits.ADCS = 0x01;    //A/D Conversion Clock bits get 2Tcy
    
    //AD Input Select Register
    AD1CHS = 0;                 //Clearing A/D Input Select Register (Good Practice)
    AD1CHSbits.CH0SA = 0b0000;  //Positive input is AN0
    
    IEC0bits.AD1IE = 1;         //Enable the A/D interrupt flag
    IFS0bits.AD1IF = 0;         //clear the A/D interrupt flag
    AD1CON1bits.ADON = 1;       //Start ADC
        
    lcd_setCursor(0,1);         //Set display cursor to display on bottom row
    lcd_printStr("Volts");      //Print "Volts" on bottom row
}

//TMR2 ISR used for updating the LCD
void __attribute__((interrupt, auto_psv)) _T2Interrupt()
{
    int avgVal = getAvg(Buffer);    //Get average value of buffer elements                  
    char adStr[20];                 //define new string for LCD top row
    lcd_setCursor(0,0);             //Set cursor to display on top row
    
    //Convert digital value to character to be displayed as string
    sprintf(adStr,"%6.4f",(3.3026/1024)*avgVal);
    
    lcd_printStr(adStr);            //Display floating value of voltage value on top row
    
    _T2IF = 0;                      //Resets Timer 2 interrupt flag
}

// Example code for A/D ISR:
void __attribute__ ((__interrupt__)) _ADC1Interrupt(void)
{
    while(!IFS0bits.AD1IF);         //Wait for finish
    int ADCValue = ADC1BUF0;        //Int variable to store the sampled value
    putVal(ADCValue);               //Add new sampled value to buffer
    IFS0bits.AD1IF = 0;             //Clear the ADC ISR flag
}
//Function that displays a string of characters on LCD
void lcd_printStr(char s[])
{
    int aSize = 0;                  //variable for counting elements in array
    
    //Loop to count elements in array
    while(s[aSize] != 0b00000000)                   
    {                     
        aSize++;                    //increment element count
    }
    
    I2C2CONbits.SEN = 1;            //Starts the Data Packet
    while(I2C2CONbits.SEN == 1);    //Wait for the start to end
    IFS3bits.MI2C2IF = 0;           //Clear the I2C2 interrupt flag
    
    I2C2TRN = 0b01111100;           //Slave Address, R/W, and Write = 0;
    while(IFS3bits.MI2C2IF == 0);   //Wait for last address command to send
    IFS3bits.MI2C2IF = 0;           //Clear the I2C2 interrupt flag
    
    //loop here to write the data and continue writing, Co = 1
    int Num = 0;
    
    for(Num=0; Num < (aSize - 1); Num++)
    {
        I2C2TRN = 0b11000000;           //Co = 1, Rs = 1, "Keep sending Data"
        while(IFS3bits.MI2C2IF == 0);   //Wait for control command to send
        IFS3bits.MI2C2IF = 0;           //Clear the I2C2 interrupt flag
        
        I2C2TRN = s[Num];               //Sends individual array elements to be written
        while(IFS3bits.MI2C2IF == 0);   //Wait for data command to send
        IFS3bits.MI2C2IF = 0;           //Clear the I2C2 interrupt flag
    }
    
    I2C2TRN = 0b01000000;           //"Last Byte", Co = 1 and Rs = 1
    while(IFS3bits.MI2C2IF == 0);   //Wait for last command data to send
    IFS3bits.MI2C2IF = 0;           //Clear the I2C2 interrupt flag
    
    I2C2CONbits.PEN = 1;            //Send Stop bit
    while(I2C2CONbits.PEN == 1);    //Wait for stop bit to go through 
}

void main(void)
{
    setup();    //Calling the setup and initialization functions
    
    
    //infinite while loop just to keep program running
    while(1)
    {      
    }
}