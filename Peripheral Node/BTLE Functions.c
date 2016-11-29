// BTLE2 Functions


#include "UART.h"
#include "tm4c123gh6pm.h"
#include "stdint.h"
#include "UART.h"

#define SWAKE	(*((volatile unsigned long *)0x40004080))			// PA5 (output)
#define CMD		(*((volatile unsigned long *)0x40005010))			// PB2 (output)
#define CONN	(*((volatile unsigned long *)0x40005080))			// PB5 (input)

#define SWAKE_HIGH	0x00000020
#define	CMD_HIGH		0x00000004


char responsetemp[10];

// Client Service List (LS Command)
void Get_ServerServiceList(char *bufPt, unsigned short max){ 
	UART1_OutString("LS");
	UART1_OutChar(CR);
	UART1_InList(bufPt, max);	
}

// Client Service List
void Get_ClientServiceList(char *bufPt, unsigned short max){ 
	UART1_OutString("LC");
	UART1_OutChar(CR);
	UART1_InList(bufPt, max);	
}

// Set SWAKE pin = 1 (will return "CMD")
void Set_SWAKE(char *bufPt){
	SWAKE = SWAKE_HIGH;
	UART1_InString(bufPt, 10);
}

// Clear SWAKE pin = 0 (will return "END")
void Clear_SWAKE(char *bufPt){
	SWAKE = 0;
	UART1_InString(bufPt, 10);
}

// Set MLDP Mode (will return "MLDP")
void Set_MLDP(char *bufPt){
	CMD = CMD_HIGH;
	UART1_InString(bufPt, 10);
}
// Clear MLDP mode (will retrun "CMD")
void Clear_MLDP(char *bufPt){
	CMD = 0;
	UART1_InString(bufPt, 10);
}

// Configure BTLE Module as Central Role and MLDP support
void Config_CentralRole(char *bufPt, unsigned short max){
	UART1_OutString("SF,1");							// Reset factory defaults
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);
	UART1_OutString("SS,60000000");				// Set services supported (40000000 = battery, 20000000 = heart rate)
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);				
	UART1_OutString("SR,90000000");				// 80000000 - central + 10000000 - MLDP
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);
	UART1_OutString("R,1");								// Reboot
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);						
	UART1_InString(bufPt, max);
}

// Configure BTLE Module as Peripheral Role and MLDP support
void Config_PeripheralRole(char *bufPt, unsigned short max){
	UART1_OutString("SF,1");							// Reset factory defaults
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);
	UART1_OutString("SS,60000000");				// Set services supported (40000000 = battery, 20000000 = heart rate)
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);				
	UART1_OutString("SR,30000000");				// 00000000 = peripheral + 20000000 = Auto-advertise + 10000000 = MLDP support
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);
	UART1_OutString("R,1");								// Reboot
	UART1_OutChar(CR);
	UART1_InString(bufPt, max);			
	UART1_InString(bufPt, max);
}

// Wait for BTLE connection
void Wait_Connection(char *bufPt){
		while ((CONN & 0x20) == 0){};
				UART1_InString(bufPt, 20);
}

void Central_Scan(char *bufPt, unsigned short max){
	UART1_OutString("F");
	UART1_OutChar(CR);
	UART1_InString(responsetemp,10);
	UART1_InString(bufPt, max);	
}

void Request_Connection(void){
	UART1_OutString("E,0,001EC024963A");
	UART1_OutChar(CR);
	UART1_InString(responsetemp,10);
}

void Get_Firmware_Version(char *bufPt){
	UART1_OutString("V");
	UART1_OutChar(CR);
	UART1_InString(bufPt,100);
}
