#include "mbed.h"
#include "hcsr04.h"
#include "Adafruit_SSD1306.h"
#include "motordriver.h"
#include "DHT22.h"

#define COOLER 1
#define HEAT 2
#define MAX_RANGE 0.00125
#define MIN_RANGE 0.00333

Motor mt(D11,PC_8); // Motor
PwmOut sound(PC_9); //Buzzer
DHT22 sensor (PB_2); //Temp & Humid
//Button
DigitalIn left_button(PA_14); //left button
DigitalIn center_button(PB_7); // center button
DigitalIn right_button(PC_4);// right button

//LED
DigitalOut left_led(PA_13); //left led
DigitalOut center_led(D6); //center led
DigitalOut right_led(PA_4); //right led

//RGB
DigitalOut Red(A1); //RGB Red
DigitalOut Green(PC_6); //RGB Green
DigitalOut Blue(A3); //RGB Blue

void config();
void ultrasonic();
void display();
void SYSTEM_OFF();
void SYSTEM_ON();
void WIND_ON(int wind_power);
void WIND_OFF();
void timeout_SYSTEM_OFF();
void BUZZWER_WARNING();
void check_temp_and_humid();

class I2CPreInit : public I2C{
   public:
   I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl){
      frequency(400000);
      start();
   };
};

Serial pc(USBTX,USBRX);
//display
I2C myI2C(I2C_SDA,I2C_SCL);
Adafruit_SSD1306_I2c myGUI(myI2C, D13, 0x78, 64, 128);
// ultrasonic pin
DigitalOut trig_pin(D10);
DigitalIn echo_pin(D7);
HCSR04 hcsr(D10, D7); 

Ticker display_ticker;
Timeout myTimeout;

bool ultrasonic_timeout_flag = false;
bool sys_on=false;
int mode=COOLER; //COOLER, HEAT => default COOLER
float current_temp, desired_temp=18.0; //default = 18.0; 
float humid;
int wind_power=2; // 1~5
bool automatic=true;

int main(){
   config();

   while(1)
   {
		 	if(!right_button){
				sys_on=!sys_on;							
				right_led=1;
				center_led=1;
				left_led=1;
				Red=0;
				Blue=1;
				Green=0;
				WIND_ON(wind_power);
			}
      while(sys_on){
        ultrasonic();
				WIND_ON(wind_power);
				BUZZWER_WARNING();
         // click SW2 : left button = cooler / heater
         if(!left_button){
            if(mode==COOLER){
               Red=1;
               Blue=0;
               Green=0;
               mt.backward(wind_power);
            }
            else{
               Red=0;
               Blue=1;
               Green=0;
               mt.forward(wind_power);
            }
         }
			//click SW9 : center button = auto / manual
				if(!center_button){
            if(automatic)
            {
               automatic=false;
               center_led=0;
            }
            else{
               automatic=true;
               center_led=1;
            }
				}

			//click SW 10 : right button = power on/off
				if(!right_button){
            sys_on=!sys_on;
            if(!sys_on){
               right_led=0;
               center_led=0;
               left_led=0;
               Red=0;
               Blue=0;
               Green=1;
               break;
            }else{
               right_led=1;
               center_led=1;
               left_led=1;
               Red=0;
               Blue=1;
               Green=0;
               mt.forward(wind_power);
            }
				}
				wait(0.1);
      }
		wait(0.5);
  }
}

void config(){
	pc.baud(9600);
	myGUI.clearDisplay();
	echo_pin.mode(PullDown); //ultrasonic
	display_ticker.attach(&display,1.0);	
	pc.printf("HAVC SYSTEM\r\n");		
	check_temp_and_humid();
}

void timeout_SYSTEM_OFF(){
    pc.printf("Human not detected!! System off\r\n");
    SYSTEM_OFF();
      ultrasonic_timeout_flag = false;
}

void ultrasonic(){
   // pc.printf("ultrasonic() started...\r\n");
   
   hcsr.start();
   wait_ms(500);
   int distance = hcsr.get_dist_cm();
   pc.printf("Distance(cm): %d\r\n", distance);
   
   if (distance >= 50 && !ultrasonic_timeout_flag){
      //pc.printf("[1] distance >= 50 && timeout is NOT activated\r\n");
      myTimeout.attach(&timeout_SYSTEM_OFF, 5.0);
      ultrasonic_timeout_flag = true;
      //pc.printf("timeout attach\r\n");
   }
   else if (distance >= 50 && ultrasonic_timeout_flag){
      //pc.printf("[2] distance >= 50 && timeout is activated\r\n");
      // pc.printf("timeout already attached\r\n");
   }
   else if (distance < 50 && !ultrasonic_timeout_flag){
      //pc.printf("[3] distance < 50 && timeout is NOT activated\r\n");
   }
   else if (distance < 50 && ultrasonic_timeout_flag){
      //pc.printf("[4] distance < 50 && timeout is activated\r\n");
      myTimeout.detach();
      ultrasonic_timeout_flag = false;
      //pc.printf("timeout detach\r\n");
   }
   wait(0.5);
}

void display(){
   myGUI.clearDisplay();
   myGUI.setTextCursor(0,0);
	wait(0.1);
   //system on/off

   if(sys_on){
      myGUI.printf ("HVAC power is on\r\n");
		 //conditioner mode COOLER/HEATER
			if(mode == COOLER){
				myGUI.printf ("MODE : Cooler\r\n");
			}
			else{
				myGUI.printf ("MODE : Heater\r\n");
			}      

			//Temperature Cur/Desired
			myGUI.printf ("Current Temp : %.1f\r\nDesired Temp : %.1f\r\n", current_temp, desired_temp);
			//Humid
			myGUI.printf("Current Humid : %.lf\r\n", humid);
			//Wind power
			myGUI.printf("Wind : ");
			for(int i=0;i<wind_power;i++){
				myGUI.printf (">");
			}

			// Air vent
			myGUI.printf ("\r\n");
   
   }
   else{
      myGUI.printf ("HVAC power is off\r\n");
   }
   
   wait(0.1);
   //myGUI.printf ("%ux%u OLED Display\r\n",myGUI.width(), myGUI.height());
   myGUI.display();
}

void SYSTEM_OFF(){
	sys_on = !sys_on;
	sound.period(1.0);
	sound=0.5;
	mode=HEAT;
	automatic=false;
}

void SYSTEM_ON(){
	sys_on = !sys_on;
	mode=COOLER;
	automatic=true;
}

void WIND_ON(int wind_power){
	if(mode==COOLER){
		mt.forward(float(wind_power)/5);
	}else{
		mt.backward(float(wind_power)/5);
	}
}
void WIND_OFF(){
	mt.stop();
}

void BUZZWER_WARNING(){
	float max=MAX_RANGE;
	float min=MIN_RANGE;
	wait(0.5);
	sound.period(max);
	sound=0.5;
	wait(3);
	sound.period(1.0);
	sound=0.5;
	wait(2);
	sound.period(min);
	sound=0.5;
	wait(3);
	sound.period(1.0);
	sound=0.5;
}

void check_temp_and_humid(){
		float h=0.0f, c=0.0f;
		sensor.sample();
		c=sensor.getTemperature()/10;
		h=sensor.getHumidity()/10;
		current_temp=c;
		humid=h;
}
