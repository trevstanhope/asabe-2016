//#include <SoftwareSerial.h>
//SoftwareSerial mySerial(2,3);   // 2 = Rx  3 = Tx
int X = 0;
int Y = 0;
boolean Color = 0;
unsigned int Green_Count = 0;
unsigned int Orange_Count = 0;
char Time_Stop[] = "00:00:00";
char output_buffer[128];

void setup() {
  //mySerial.begin(9600);
  Serial.begin(9600);
}

void loop() {
if (Green_Count < 8 && Orange_Count < 8) {
  Green_Count++;
  Orange_Count++;
  clock_stop(Green_Count,Orange_Count,Time_Stop);
}
}

void clock_stop(unsigned int green_count, unsigned int orange_count, char time_stop[]) {
  //sprintf(output_buffer,"%d,%d,%d,%d,%d,%s\r\n",X,Y,Color,green_count,orange_count,time_stop);
  sprintf(output_buffer,"0,0,0,%d,%d,%s\r\n",X,Y,Color,green_count,orange_count,time_stop);
  Serial.println(output_buffer);
  //Serial.flush(); 
}

