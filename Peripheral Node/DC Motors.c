// DC Motor using PWM modules
// Using M0PWM0 and M0PWM1 controlled by Module 0 PWM Generator 0
// MOPWM0 Pin 1, PB6 (4)	- Enable L293D (pin 1)
// M0PWM1 Pin 4, PB7 (4)	- Enable L293D  (pin9)

// Left Side Control
// PB6 - L293D (pin1)
// PE0 - L293D (pin 2)
// PE1 - L293D (pin 7)

// Right Side Control
// PB7 - L293D (pin 9)
// PE2 - L293D (pin 10)
// PE3 - L293D (pin 15)


// 80MHz clock with TExaS.h
// PWM clock = 50MHz /64 = 781250Hz

// Pre-processor Directives Section
#include <stdint.h>
#include "tm4c123gh6pm.h"

#define TenPercentSpeed 78
#define Neutral 1171
#define LeftMax 1561
#define LeftMid	1366
#define RightMax 780
#define RightMid	975


void PWM_Init(void){
	SYSCTL_RCGC0_R |= 0x00100000;           			// activate PWM0
  SYSCTL_RCGCGPIO_R |= 0x0002;          			// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};			// ready?		
  GPIO_PORTB_AFSEL_R |= 0xD0;           			// enable alt funct on PB4,6,7
	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0x00F0FFFF)+0x44040000;
  GPIO_PORTB_AMSEL_R &= ~0xD0;          			// disable analog functionality on PB4,6,7
	GPIO_PORTB_DIR_R |= 0xD0;										// diretion outputPB4, PB6, PB7
	GPIO_PORTB_DEN_R |= 0xD0;             			// enable digital I/O on PB4,6,7
		
	SYSCTL_RCC_R |= 0x001E0000;										// Bit 20 = Use PWMDIV Bit 19:17 = 7 which is /64 divisor (default)		
	// PWM Clock = 781250Hz	
	
	PWM0_0_CTL_R = 0x00;				// Disable M0PWM0 and M0PWM1
	PWM0_1_CTL_R = 0x00;
		
	PWM0_0_GENA_R = 0x000000C8;	// Set PWM0 Gen 0 settings
	PWM0_0_GENB_R = 0x00000C08;
		
	PWM0_1_GENA_R = 0x000000C8;	// Set PWM0 Gen 1 settings
	PWM0_1_GENB_R = 0x00000C08;
		
	PWM0_0_LOAD_R  = 781; 									// PWM Clock = 781250Hz, DC motor period = 1ms or 1kHz LOAD Value = 781250/1000 = 781
	PWM0_1_LOAD_R = 15624;									// PWM Clock = 781250Hz DC motor = 50Hz so 781250Hz/50Hz = 15625 - 1 = 15624 = 0x3D08
																	// 10,000-1 = PWM LOAD
																	// 5,000-1 = 50% Duty Cycle
	//PWM0_0_CMPA_R = 0x00001387;			// 50% Duty Cycle
	//PWM0_0_CMPB_R = 0x000009C3;			// 25% Duty Cycle
	
	PWM0_0_CMPA_R = 0;			// Initialize to 0% duty cycle (no speed) 390 = 50% Duty Cycle
	PWM0_0_CMPB_R = 0;			// Initialize to 0% duty cycle (no speed)  78 = 10% Duty Cycle
	
	PWM0_1_CMPA_R = Neutral;			// Servo motor initialize to neutral
	
	PWM0_0_CTL_R = 0x01;							// Start PWM Timers
	PWM0_1_CTL_R = 0x01;
	PWM0_ENABLE_R = 0x07;				// Enable M0PWM0 and M0PWM1 and M0PWM2
}



void ControlSpeed(char speed){
	unsigned long temp;
	temp = (unsigned long) speed;
	if (speed == 0){
		PWM0_0_CMPA_R = 0;
		PWM0_0_CMPB_R = 0;
	} else {
		PWM0_0_CMPA_R = (TenPercentSpeed * temp);
		PWM0_0_CMPB_R = (TenPercentSpeed * temp);
	}
}

void ControlDirection(char direction){
	if (direction == 0x05){
		PWM0_1_CMPA_R = Neutral;	
	}
	if (direction == 0x01){
		PWM0_1_CMPA_R = LeftMax;	
	}
	if (direction == 0x09){
		PWM0_1_CMPA_R = RightMax;
	}	
	if (direction == 0x03){
		PWM0_1_CMPA_R = LeftMid;	
	}
	if (direction == 0x07){
		PWM0_1_CMPA_R = RightMid;
	}		
}

unsigned long ControlVertical(char vertical){
	unsigned long output;
	switch (vertical){
		case 0x05:
			output = 0x00;
			break;
		case 0x01:
			output = 0x0A;	// reverse
			break;
		case 0x09:
			output = 0x05; // forward
			break;
		default:
			output = 0x00;
			break;
	}
	return output;
	
	/*
	if (vertical == 0x05){
		PWM0_0_CMPA_R = 0;			// 0% duty cycle
		PWM0_0_CMPB_R = 0;	
		GPIO_PORTE_DATA_R = 0x00;
	}
	if (vertical == 0x09){			// forward speed 4
		PWM0_0_CMPA_R = 780;			// 100% duty cycle
		PWM0_0_CMPB_R = 780;	
		GPIO_PORTE_DATA_R = 0x05;
	}
	if (vertical == 0x08){			// forward speed 3
		PWM0_0_CMPA_R = 585;			// 75% duty cycle
		PWM0_0_CMPB_R = 585;	
		GPIO_PORTE_DATA_R = 0x05;	
	}	
	if (vertical == 0x07){			// forward speed 2
		PWM0_0_CMPA_R = 390;			// 50% duty cycle
		PWM0_0_CMPB_R = 390;	
		GPIO_PORTE_DATA_R = 0x05;	
	}
	if (vertical == 0x06){			// forward speed 1
		PWM0_0_CMPA_R = 195;			// 25% duty cycle
		PWM0_0_CMPB_R = 195;	
		GPIO_PORTE_DATA_R = 0x05;		
	}
	if (vertical == 0x04){			// reverse speed 1
		PWM0_0_CMPA_R = 195;			// 25% duty cycle
		PWM0_0_CMPB_R = 195;	
		GPIO_PORTE_DATA_R = 0x0A;	
	}
	if (vertical == 0x03){			// reverse speed 2
		PWM0_0_CMPA_R = 390;			// 50% duty cycle
		PWM0_0_CMPB_R = 390;	
		GPIO_PORTE_DATA_R = 0x0A;	
	}	
	if (vertical == 0x02){			// reverse speed 3
		PWM0_0_CMPA_R = 585;			// 75% duty cycle
		PWM0_0_CMPB_R = 585;	
		GPIO_PORTE_DATA_R = 0x0A;	
	}
	if (vertical == 0x01){			// reverse speed 4
		PWM0_0_CMPA_R = 780;			// 100% duty cycle
		PWM0_0_CMPB_R = 780;	
		GPIO_PORTE_DATA_R = 0x0A;	
	}	
	*/
}

void ControlHorizontal(char horizontal){
	switch (horizontal){
		case 0x05:
			PWM0_1_CMPA_R = Neutral;
			break;
		case 0x09:
			PWM0_1_CMPA_R = LeftMax;	
			break;
		case 0x01:
			PWM0_1_CMPA_R = RightMax;
			break;
		case 0x07:
			PWM0_1_CMPA_R = LeftMid;	
			break;
		case 0x03:
			PWM0_1_CMPA_R = RightMid;
			break;
		default:
			PWM0_1_CMPA_R = Neutral;
			break;
	}
	
/*	
	if (horizontal == 0x05){
		PWM0_1_CMPA_R = Neutral;	
	}
	if (horizontal == 0x09){
		PWM0_1_CMPA_R = LeftMax;	
	}
	if (horizontal == 0x01){
		PWM0_1_CMPA_R = RightMax;
	}	
	if (horizontal == 0x07){
		PWM0_1_CMPA_R = LeftMid;	
	}
	if (horizontal == 0x03){
		PWM0_1_CMPA_R = RightMid;
	}
*/	
}
