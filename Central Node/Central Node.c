

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

// Main Clock = 50MHz

#include "PLL.h"
#include "UART.h"
#include "stdint.h"
#include "tm4c123gh6pm.h"
#include "BTLE Functions.h"
#include "ADC.h"


#define CONN	(*((volatile unsigned long *)0x40005080))			// PB5 (input)

// Global Variables

char temp[200];
char ServerServiceList[200];
char ClientServiceList[200];
char PeripheralInfo[50];
char MACaddress[20];
char StatusConnection[20];
char StatusMLDP[20];


	char speed, speed_prev;
	char direction, direction_prev;
	char vertical, vertical_prev;			// forward, reverse, speed
	char horizontal, horizontal_prev; // direction
	char effects;
	
	char PF0_Button, PF0_Button_Prev;
	char PF4_Button, PF4_Button_Prev;
	
	unsigned long ADCdata, ADChorizontal, ADCvertical;


// Speed between 0 and 9
// Direction between 1 and 5
// Direction 1 = hard left
// Direction 2 = left
// Direction 3 = straight
// Direction 4 = right
// Direction 5 = hard right


// Prototypes
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Make_Connection(void);
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
	GPIO_PORTB_AMSEL_R &= ~0x24;      // 3) disable analog on PB2, PB5
  GPIO_PORTB_PCTL_R &= ~0x00F00F00; // 4) PCTL GPIO on PA5
  GPIO_PORTB_DIR_R &= ~0x20;        // 5) direction PB5 input
	GPIO_PORTB_DIR_R |= 0x04;							// direction PB2 output
  GPIO_PORTB_AFSEL_R &= ~0x24;      // 6) PB2, PB5 regular port function
  GPIO_PORTB_DEN_R |= 0x24;         // 7) enable PB2, PB5 digital port
	
	// Initialize PA5 (SWAKE - output)
	SYSCTL_RCGC2_R |= 0x00000001;     // 1) activate clock for Port A
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTA_AMSEL_R &= ~0x20;      // 3) disable analog on PA5
  GPIO_PORTA_PCTL_R &= ~0x00F00000; // 4) PCTL GPIO on PA5
  GPIO_PORTA_DIR_R |= 0x20;        // 5) direction PA5 input
  GPIO_PORTA_AFSEL_R &= ~0x20;      // 6) PA5 regular port function
  GPIO_PORTA_DEN_R |= 0x20;         // 7) enable PA5 digital port
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

  PLL_Init();								// 50MHz System Clock
	UART0_Init();
  UART1_Init();              // initialize UART
	ADC0_Init();
	GPIO_Init();
	PortF_Init();
	Timer2_Init(0x0FFFFFFF);
	EnableInterrupts();
	
	// Central Node
	
	// Enable Timer2 - If no connection is made after Timer2 triggers, then software reset chip
	NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable TIMER2A
	
	// Make Connection with peripheral
	Make_Connection();
	
	// When Connection is made, disable Timer2
	Timer2A_Stop();
	
	effects = 0x00;
	
  while(1){

/*		
		// Read LaunchPad switches and send out Direction and Speed info over UART
		PF0_Button = (GPIO_PORTF_DATA_R & 0x01);
		PF4_Button = (GPIO_PORTF_DATA_R & 0x10);
		
		// Read ADC (slide pot)
		ADCdata = ADC0_In();
		
		// Get direction data from ADC
		if (ADCdata <= 820){
			direction = 0x01;
		}
		if ((ADCdata > 820) && (ADCdata <= 1640)){
			direction = 0x03;
		}
		if ((ADCdata > 1640) && (ADCdata <= 2460)){
			direction = 0x05;
		}
		if ((ADCdata > 2460) && (ADCdata <= 3280)){
			direction = 0x07;
		}
		if (ADCdata >= 3280){
			direction = 0x09;
		}
		
		// Read button presses
		if ((PF0_Button == 0) && (PF0_Button != PF0_Button_Prev)){				// switch is logic low
			if (speed < 9){
				speed++;
			}
		}
		if ((PF4_Button == 0) && (PF4_Button != PF4_Button_Prev)){				// switch is logic low
			if (speed > 0){
				speed--;
			}
		}
		
		// if new speed or direction data is available then send (otherwise do nothing)
		if ((speed != speed_prev) || (direction != direction_prev)){
			UART1_OutChar(speed);
			UART1_OutChar(direction);
		}
		
		Delay1ms(1);
		PF0_Button_Prev = PF0_Button;
		PF4_Button_Prev = PF4_Button;
		speed_prev = speed;
		direction_prev = direction;
*/		
		// Read PF0 Switch
		PF0_Button = (GPIO_PORTF_DATA_R & 0x01);
		//PF4_Button = (GPIO_PORTF_DATA_R & 0x10);

		// Read ADC (slide pot)
			// For joystick input (PE4, PE5)
			// PE4	Right		Ain9		Horizonal
			// PE5	Left		Ain8		Vertical
		ADC0_In(&ADCvertical, &ADChorizontal);
		
		// Get horizonal data from ADC		
		if (ADChorizontal <= 820){
			horizontal = 0x01;
		}
		if ((ADChorizontal > 820) && (ADChorizontal <= 1640)){
			horizontal = 0x03;
		}
		if ((ADChorizontal > 1640) && (ADChorizontal <= 2460)){
			horizontal = 0x05;
		}
		if ((ADChorizontal > 2460) && (ADChorizontal <= 3280)){
			horizontal = 0x07;
		}
		if (ADChorizontal > 3280){
			horizontal = 0x09;
		}
		
				// Get horizonal data from ADC
		if (ADCvertical <= 820){
			vertical = 0x01;
		}
		if ((ADCvertical > 820) && (ADCvertical <= 1640)){
			vertical = 0x01;
		}
		if ((ADCvertical > 1640) && (ADCvertical <= 2460)){
			vertical = 0x05;
		}
		if ((ADCvertical > 2460) && (ADCvertical <= 3280)){
			vertical = 0x09;
		}
		if (ADCvertical > 3280){
			vertical = 0x09;
		}
		
		// Read button presses
		if ((PF0_Button == 0) && (PF0_Button != PF0_Button_Prev)){				// switch is logic low
			effects ^= 0x01;
		}
		
		
		// if new horizonal or vertical data is available then send (otherwise do nothing)
		if ((horizontal != horizontal_prev) || (vertical != vertical_prev) || (PF0_Button != PF0_Button_Prev)){
			UART1_OutChar(vertical);
			UART1_OutChar(horizontal);
			UART1_OutChar(effects);
		}
		
		Delay1ms(1);
		vertical_prev = vertical;
		horizontal_prev = horizontal;
		PF0_Button_Prev = PF0_Button;
	
		
  } // while loop
}

void Make_Connection(void){
		// Set SWAKE output pin high to BTLE module
	Set_SWAKE(temp);

	// Config as Central Role 
	Config_CentralRole(temp, 200);
	
	// Send Command to collect information on Peripheral Device
	Central_Scan(PeripheralInfo, 50);
	
	// Request connection with peripheral device
	Request_Connection();
	
	// Wait for Connection
	Wait_Connection(StatusConnection);

	// get Server and Client services lists
	Get_ServerServiceList(ServerServiceList, 200);
	Get_ClientServiceList(ClientServiceList, 200);

	// Go into MLDP mode
	Set_MLDP(StatusMLDP);
}

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
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}

void Timer2A_Handler(void){ 
  TIMER2_ICR_R = 0x00000001;   // acknowledge timer2A timeout
	
	if ((CONN & 0x20) == 0){
		NVIC_APINT_R = 0x05FA0004;	// if no connection, then reset chip
	}
}

void Timer2A_Stop(void){
  TIMER2_CTL_R &= ~0x00000001; // disable
}

void Timer2A_Start(void){
  TIMER2_CTL_R |= 0x00000001;   // enable
}

