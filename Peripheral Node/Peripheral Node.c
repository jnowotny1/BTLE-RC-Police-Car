// BTLE Remote Control Car 

// Main Clock = 50MHz

// DC Gear motors
// Using M0PWM0 and M0PWM1 controlled by Module 0 PWM Generator 0
// MOPWM0 Pin 1, PB6 (4)	Left Side
// M0PWM1 Pin 4, PB7 (4)	Right Side

// Piezo Buffer on PA7
// LED output PA2, PA3

// Servo motor
// 

// BTLE
// UART1 PB0 RX PB1 TX

#include "PLL.h"
#include "UART.h"
#include "stdint.h"
#include "tm4c123gh6pm.h"
#include "BTLE Functions.h"
#include "DC Motors.h"


#define BUZZER	(*((volatile unsigned long *)0x40004200))			// PA7 (output)
#define CONN		(*((volatile unsigned long *)0x40005080))			// PB5 (input)
#define LED_PA2	(*((volatile unsigned long *)0x40004010))			// PA2 (output)
#define	LED_PA3 (*((volatile unsigned long *)0x40004020))			// PA3 (output)
#define PERIOD_LOW	27780
#define PERIOD_HIGH	55560

// Global Variables
	char temp[200];
	char ServerServiceList[200];
	char ClientServiceList[200];
	char MACaddress[20];
	char StatusConnection[20];
	char StatusMLDP[20];
	char UARTinput;
	char speed;
	char direction;
	char vertical;
	char horizontal;
	char effects;
	char LED2timer;
	char LED3timer;
	unsigned long driveoutput;
	unsigned long period;
	unsigned long period_direction;						// 0 = frequency going up, 1 frequency going down
	


// Prototypes
void DisableInterrupts(void);
void EnableInterrupts(void);
void Timer2_Init(unsigned long period);
void Timer2A_Handler(void);
void Timer2A_Stop(void);
void Timer2A_Start(void);

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART0_OutChar(CR);
  UART0_OutChar(LF);
}
//debug code

void GPIO_Init(void){unsigned long delay;
	// Initialize PB2 (CMD - output) PB5 (Connect - input)
  SYSCTL_RCGC2_R |= 0x00000002;     // 1) activate clock for Port B
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
	GPIO_PORTB_AMSEL_R &= ~0x24;      // 3) disable analog on PB2,PB5, PB6
  GPIO_PORTB_PCTL_R &= ~0x00F00F00; // 4) PCTL GPIO on PB2, PB5
  GPIO_PORTB_DIR_R &= ~0x20;        // 5) direction PB5 input
	GPIO_PORTB_DIR_R |= 0x04;							// direction PB2 output
  GPIO_PORTB_AFSEL_R &= ~0x24;      // 6) PB2, PB5 regular port function
  GPIO_PORTB_DEN_R |= 0x24;         // 7) enable PB2, PB5 digital port
	
	// Initialize PA5 (SWAKE - output) and PA7 (Buzzer - Output) and PA2, PA3 (LED outputs)
	SYSCTL_RCGC2_R |= 0x00000001;     // 1) activate clock for Port A
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTA_AMSEL_R &= ~0xAC;      // 3) disable analog on PA5, PA7
  GPIO_PORTA_PCTL_R &= ~0xF00FFF00; // 4) PCTL GPIO on PA5, PA7
  GPIO_PORTA_DIR_R |= 0xAC;        // 5) direction PA5 output, PA7 output
  GPIO_PORTA_AFSEL_R &= ~0xAC;      // 6) PA5, PA7 regular port function
  GPIO_PORTA_DEN_R |= 0xAC;         // 7) enable PA5, PA7 digital port
	
	// Initialize PE0-PE3
	SYSCTL_RCGC2_R |= 0x0010;          			// activate port E
  delay = SYSCTL_RCGC2_R;			// ready?	
  GPIO_PORTE_AMSEL_R &= ~0x0F;        							// 3) disable analog on PE4-0
	GPIO_PORTE_PCTL_R &= ~0x0000FFFF;   							// 4) PCTL GPIO on PE4-0
  GPIO_PORTE_DIR_R |= 0x0F;          							// 5) PE4-0 outputs
  GPIO_PORTE_AFSEL_R &= ~0x0F;        							// 6) disable alt funct on PF7-0
	//GPIO_PORTE_PUR_R |= 0x0F;          							// enable pull-up on PE4-0
  GPIO_PORTE_DEN_R |= 0x0F;          							// 7) enable digital I/O on PF4,PF3,PF2,PF1,PF0
}

void PortF_Init(void){volatile unsigned long delay;				// Initialize PortF
  SYSCTL_RCGC2_R |= 0x00000020;     							// 1) activate clock for Port F
  delay = SYSCTL_RCGC2_R;           							// allow time for clock to start
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   							// 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           							// allow changes to PF4-0
  // only PF0 needs to be unlocked, other bits can't be locked
  GPIO_PORTF_AMSEL_R = 0x00;        							// 3) disable analog on PF
  GPIO_PORTF_PCTL_R = 0x00000000;   							// 4) PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R = 0x0E;          							// 5) PF3, PF2, PF1 outputs
  GPIO_PORTF_AFSEL_R = 0x00;        							// 6) disable alt funct on PF7-0
	GPIO_PORTF_PUR_R = 0x11;          							// enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R |= 0x1F;          							// 7) enable digital I/O on PF4,PF3,PF2,PF1,PF0
}

void Delay1ms(unsigned long count){unsigned long volatile time;
  while(count>0){
    time = 17000;  // 0.001s at 16MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

int main(void){
	
	period = PERIOD_LOW; // initialize timer period
	
	// Initialize Peripherals
  PLL_Init();								// 50MHz System Clock
	//UART0_Init();							// Debug (output to HyperTerminal)
  UART1_Init();             // initialize UART1 for BTLE Communication
	GPIO_Init();							// initialize GPIO
	PortF_Init();							// initialize PORTF
	PWM_Init();								// initialize PWM (Gear motors and Servo motor)
	Timer2_Init(period);						// initialize Timer2 = 900Hz
	
	// Initialize Variables

	effects = 0x00;		// initialized off
	LED2timer = 50;
	LED3timer = 0;
	
	// Peripheral Node
	
	Set_SWAKE(temp);
	
	//Get_Firmware_Version(temp);

	Config_PeripheralRole(temp, 200);

	Wait_Connection(StatusConnection);
	

	Get_ServerServiceList(ServerServiceList, 200);
	Get_ClientServiceList(ClientServiceList, 200);
	
	Set_MLDP(StatusMLDP);
	

  while(1){
/*		
		// Read in 2 UART char inputs (ist speed, 2nd direction)
		speed = UART1_InChar();
		direction = UART1_InChar();
		
		// Modify output for speed and direction
		ControlSpeed(speed);
		ControlDirection(direction);

*/		
		// Using Joystick 
		// Read in 2 UART char inputs (ist speed, 2nd direction)
		vertical = UART1_InChar();
		horizontal = UART1_InChar();
		effects = UART1_InChar();
		
		// Modify output for speed and direction
		driveoutput = ControlVertical(vertical);
		ControlHorizontal(horizontal);
		
		// Output drive controls
		GPIO_PORTE_DATA_R = driveoutput;
		
		// Control Effects
		if (effects == 0x01){
			Timer2A_Start();
		} else {
			Timer2A_Stop();
			BUZZER = 0x00;
			LED_PA2 = 0x00;
			LED_PA3 = 0x00;
		}
	}
}





	/* // Test UART input code
	temp[0] = UART1_InChar();
	temp[1] = UART1_InChar();
	temp[2] = UART1_InChar();
	temp[3] = UART1_InChar();
	temp[4] = UART1_InChar();
	temp[5] = UART1_InChar();
	temp[6] = UART1_InChar();
	temp[7] = UART1_InChar();
	temp[8] = UART1_InChar();
	temp[9] = UART1_InChar();
*/
	
void Timer2_Init(unsigned long period){ 
  unsigned long volatile delay;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  delay = SYSCTL_RCGCTIMER_R;
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x00000000; // 8) priority 0
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
 // TIMER2_CTL_R |= 0x00000001;    // 10) enable timer2A
}

void Timer2A_Handler(void){ 
  TIMER2_ICR_R = 0x00000001;   // acknowledge timer2A timeout
	BUZZER ^= 0x80;     // toggle PA7
	
	if (period_direction == 0){
		if (period < PERIOD_HIGH){
			period += 10;
		} else {
			period_direction = 1;
		}
	}
	if (period_direction == 1){
		if (period > PERIOD_LOW){
			period -= 10;
		} else {
			period_direction = 0;
		}
	}
	
	if (LED2timer == 100){			// Toggle LED2 every 100 Timer2 interrupts
		LED_PA2 ^= 0x04;
		LED2timer = 0;
	}
	
	if (LED3timer == 100){			// Toggle LED3 every 100 Timer2 interrupts
		LED_PA3 ^= 0x08;
		LED3timer = 0;
	}
	
	LED2timer++;									// increment LED timer 2
	LED3timer++;									// increament LED timer 3
	TIMER2_TAILR_R = period-1 ;    // 4) reload value
}
		
void Timer2A_Stop(void){
  TIMER2_CTL_R &= ~0x00000001; // disable
}

void Timer2A_Start(void){
  TIMER2_CTL_R |= 0x00000001;   // enable
}


