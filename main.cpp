#include "mbed.h"
#include "hcsr04.h"
#include "Adafruit_SSD1306.h"
#include "motordriver.h"
#include "DHT22.h"

Motor mt(D11,PC_8);

#define COOLER 1
#define HEAT 2
//Button
DigitalIn SW2(PA_14); //left button
DigitalIn SW9(PB_7); // center button
DigitalIn SW10(PC_4);// right button

//LED
DigitalOut D7(PA_13); //left led
DigitalOut D8(D6); //center led
DigitalOut D9(PA_4); //right led

//RGB
DigitalOut Red(A1); //RGB Red
DigitalOut Green(PC_6); //RGB Green
DigitalOut Blue(A3);; //RGB Blue

class I2CPreInit : public I2C{
   public:
   I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl){
      frequency(400000);
      start();
   };
};

I2C myI2C(I2C_SDA,I2C_SCL);
Adafruit_SSD1306_I2c myGUI(myI2C, D13, 0x78, 64, 128);

Serial pc(USBTX,USBRX);

DigitalOut trig_pin(D10);
DigitalIn echo_pin(D7);

HCSR04 hcsr(D10, D7);
DHT22 DHT(PB_2);
Timeout myTimeout;

bool sys_on=false;
int mode=COOLER; //COOLER, HEAT => default COOLER
float current_temp=26.8, desired_temp=18.0; //default = 18.0; 
int wind_power=0.5; // 0.0~1.0
bool automatic=true;
bool ultrasonic_timeout_flag = false;

void timeout_SYSTEM_OFF();
void ultrasonic();
void display();

int main(){
	myGUI.clearDisplay();
   pc.baud(9600);
   echo_pin.mode(PullDown); //ultrasonic
	
   while(1)
   {
      display();
      while(sys_on){
         // click SW2 : left button = cooler / heater
         if(!SW10){
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
		   if(!SW9){
            if(automatic)
            {
               automatic=false;
               D8=0;
            }
            else{
               automatic=true;
               D8=1;
            }
			}
         if(automatic){
            DHT.sample();
            float c=DHT.getTemperature();
            float h=DHT.getHumidity();
            int temp = (int)c;
            int humid = (int)h;
            pc.printf("Temp: %d, Humid: %d\r\n", temp, humid);
            //AUTO Set 18.0
            if(temp-18.0>0){
               wind_power=(temp-18.0)
            }

         }
         else{

         }
			//click SW 10 : right button = power on/off
		   if(!SW10){
            sys_on!=sys_on;
            if(!sys_on){
               D9=0;
               D8=0;
               D7=0;
               Red=0;
               Blue=0;
               Green=1;
               break;
            }else{
               D9=1;
               D8=1;
               D7=1;
               Red=0;
               Blue=1;
               Green=0;
               mt.forward(wind_power);
            }

			}
         display();
      }
		   
			wait(0.5);
  }
}
void ultrasonic(){
   // pc.printf("ultrasonic() started...\r\n");
   
   hcsr.start();
   wait_ms(500);
   int distance = hcsr.get_dist_cm();
   pc.printf("Distance(cm): %d\r\n", distance);
   
   if (distance >= 50 && !ultrasonic_timeout_flag){
      //pc.printf("[1] distance >= 50 && timeout is NOT activated\r\n");
      myTimeout.attach(&timeout_SYSTEM_OFF, 3.0);
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

void timeout_SYSTEM_OFF(){
    pc.printf("TIMEOUT! task activated\r\n");
    // SYSTEM_OFF();
      ultrasonic_timeout_flag = false;
}

void display(){
   myGUI.clearDisplay();
   myGUI.setTextCursor(0,0);
	wait(0.1);
   //system on/off
   ultrasonic();
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


   
