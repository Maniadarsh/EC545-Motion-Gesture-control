//Written by Ahmet Burkay KIRNIK
//TR_CapaFenLisesi
//Measure Angle with a MPU-6050(GY-521)

#include<Wire.h>

const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

int minVal=265;
int maxVal=402;

double x;
double y;
double z;

char directions;


void setup(){
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(9600);
}
void loop(){
  bool modified = false;
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
    int xAng = map(AcX,minVal,maxVal,-90,90);
    int yAng = map(AcY,minVal,maxVal,-90,90);
    int zAng = map(AcZ,minVal,maxVal,-90,90);

       x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
       y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
       z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);

       if (z < 0 && abs(z) > 5) {
            directions = 'u';
            Serial.println(directions);
            modified = true;
        }
        else if (z > 0 && abs(z) > 5) {
            directions = 'd';
            Serial.println(directions);
            modified = true;
        }
        else if (y < 0 && abs(y) > 5) {
            directions = 'l';
            Serial.println(directions);
            modified = true;
        }
        else if (y > 0 && abs(y) > 5) {
            directions = 'r';
            Serial.println(directions);
            modified = true;
        } 
        else if (modified == false){
          Serial.println("No change");
          //make sure rf module does not send anything
        } 



     Serial.print("AngleX= ");
     Serial.println(x);

     Serial.print("AngleY= ");
     Serial.println(y);

     Serial.print("AngleZ= ");
     Serial.println(z);
     Serial.println("-----------------------------------------");
     delay(400);
}
