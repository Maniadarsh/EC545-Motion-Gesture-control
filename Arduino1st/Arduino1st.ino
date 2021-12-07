/*
 * Example of a basic FreeRTOS queue
 * https://www.freertos.org/Embedded-RTOS-Queues.html
 */

// Include Arduino FreeRTOS library / Arduino Libraries
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <Wire.h>


//  Include RF communication
#include <SPI.h>
#include <RH_NRF24.h>
#include <Kalman.h> // Source: https://github.com/TKJElectronics/KalmanFilter
/*
 * Declaring a global variable of type QueueHandle_t 
 * 
 */
QueueHandle_t charQueue;
QueueHandle_t retQueue;

/* Global Variabl for RF */
RH_NRF24 nrf24;


/*
 * Global Variable for MPU
 *
*/
#include <Kalman.h> // Source: https://github.com/TKJElectronics/KalmanFilter
#define MPU_ADDR  0x68

#define RESTRICT_PITCH

Kalman kalmanX; // Create the Kalman instances
Kalman kalmanY;

double accX, accY, accZ;
double gyroX, gyroY, gyroZ;
int16_t tempRaw;

double gyroXangle, gyroYangle; // Angle calculate using the gyro only
double compAngleX, compAngleY; // Calculated angle using a complementary filter
double kalAngleX, kalAngleY; // Calculated angle using a Kalman filter

uint32_t timer;
uint8_t i2cData[14]; // Buffer for I2C data
char gyro_state;
char LED_state;


char directions(double kalAngleX,double kalAngleY){
  char dir = '0';
  int8_t threshold = 15;
  bool xp = (kalAngleX > threshold); 
  bool xu = (kalAngleX > 5); 
  //bool yu = (kalAngleY > -20); 
  bool xn = (kalAngleX < -threshold); 
  bool xz = (kalAngleX < threshold) && (kalAngleX > -threshold);
  bool xq = (kalAngleX < 0) && (kalAngleX > -threshold);
  bool xt = (kalAngleX > 0 ) && (kalAngleY < -15);
  bool xr = (kalAngleX > 0) && (kalAngleY > 10);
  bool yp = (kalAngleY > threshold); 
  bool yc = (kalAngleY > 1);
  bool yn = (kalAngleY < -threshold); 
  bool yz = (kalAngleY < threshold) && (kalAngleY > -threshold);
  if(xp && yz) dir = 'r';
  if(xn && yz) dir = 'l';
  if(xr) dir = '/'; // upper right diagonal
  if(xn && yc) dir = '\\'; // lower right diagonal
  if(xt) dir = '*'; // upper left diagona 
  if(xn && yn) dir = '$'; // lower left diagonal 
  if(xz && yp) dir = 'u';
  if(xq && yn) dir = 'd';
  return dir;
}


char MPU6050_get_incline(){
   /* Update all the values */
  while (i2cRead(0x3B, i2cData, 14));
  accX = (int16_t)((i2cData[0] << 8) | i2cData[1]);
  accY = (int16_t)((i2cData[2] << 8) | i2cData[3]);
  accZ = (int16_t)((i2cData[4] << 8) | i2cData[5]);
  tempRaw = (int16_t)((i2cData[6] << 8) | i2cData[7]);
  gyroX = (int16_t)((i2cData[8] << 8) | i2cData[9]);
  gyroY = (int16_t)((i2cData[10] << 8) | i2cData[11]);
  gyroZ = (int16_t)((i2cData[12] << 8) | i2cData[13]);;

  double dt = (double)(micros() - timer) / 1000000; // Calculate delta time
  timer = micros();

  // Source: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf eq. 25 and eq. 26
  // atan2 outputs the value of -π to π (radians) - see http://en.wikipedia.org/wiki/Atan2
  // It is then converted from radians to degrees

  double roll  = atan2(accY, accZ) * RAD_TO_DEG;
  double pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;


  double gyroXrate = gyroX / 131.0; // Convert to deg/s
  double gyroYrate = gyroY / 131.0; // Convert to deg/s


  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((roll < -90 && kalAngleX > 90) || (roll > 90 && kalAngleX < -90)) {
    kalmanX.setAngle(roll);
    compAngleX = roll;
    kalAngleX = roll;
    gyroXangle = roll;
  } else
    kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleX) > 90)
    gyroYrate = -gyroYrate; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt);


  gyroXangle += gyroXrate * dt; // Calculate gyro angle without any filter
  gyroYangle += gyroYrate * dt;
  //gyroXangle += kalmanX.getRate() * dt; // Calculate gyro angle using the unbiased rate
  //gyroYangle += kalmanY.getRate() * dt;

  compAngleX = 0.93 * (compAngleX + gyroXrate * dt) + 0.07 * roll; // Calculate the angle using a Complimentary filter
  compAngleY = 0.93 * (compAngleY + gyroYrate * dt) + 0.07 * pitch;

  // Reset the gyro angle when it has drifted too much
  if (gyroXangle < -180 || gyroXangle > 180)
    gyroXangle = kalAngleX;
  if (gyroYangle < -180 || gyroYangle > 180)
    gyroYangle = kalAngleY;

  /* Print Data */

  return directions(kalAngleX,kalAngleY);
}


void RF_setup() 
{
  if (!nrf24.init())
    Serial.println("R1");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("R2");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("R3");    
}

void MPU_setup(){
#if ARDUINO >= 157
  Wire.setClock(400000UL); // Set I2C frequency to 400kHz
#else
  TWBR = ((F_CPU / 400000UL) - 16) / 2; // Set I2C frequency to 400kHz
#endif

  i2cData[0] = 7; // Set the sample rate to 1000Hz - 8kHz/(7+1) = 1000Hz
  i2cData[1] = 0x00; // Disable FSYNC and set 260 Hz Acc filtering, 256 Hz Gyro filtering, 8 KHz sampling
  i2cData[2] = 0x00; // Set Gyro Full Scale Range to ±250deg/s
  i2cData[3] = 0x00; // Set Accelerometer Full Scale Range to ±2g
  while (i2cWrite(0x19, i2cData, 4, false)); // Write to all four registers at once
  while (i2cWrite(0x6B, 0x01, true)); // PLL with X axis gyroscope reference and disable sleep mode

  while (i2cRead(0x75, i2cData, 1));
  if (i2cData[0] != 0x68) { // Read "WHO_AM_I" register
    Serial.print(F("M1"));
    while (1);
  }

  delay(100); // Wait for sensor to stabilize

  /* Set kalman and gyro starting angle */
  while (i2cRead(0x3B, i2cData, 6));
  accX = (int16_t)((i2cData[0] << 8) | i2cData[1]);
  accY = (int16_t)((i2cData[2] << 8) | i2cData[3]);
  accZ = (int16_t)((i2cData[4] << 8) | i2cData[5]);

  // Source: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf eq. 25 and eq. 26
  // atan2 outputs the value of -π to π (radians) - see http://en.wikipedia.org/wiki/Atan2
  // It is then converted from radians to degrees
#ifdef RESTRICT_PITCH // Eq. 25 and 26
  double roll  = atan2(accY, accZ) * RAD_TO_DEG;
  double pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  double roll  = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
  double pitch = atan2(-accX, accZ) * RAD_TO_DEG;
#endif

  kalmanX.setAngle(roll); // Set starting angle
  kalmanY.setAngle(pitch);
  gyroXangle = roll;
  gyroYangle = pitch;
  compAngleX = roll;
  compAngleY = pitch;

  timer = micros();

  gyro_state = '0';
  
}


void button_IRQ(){
   LED_state = 1;
   Serial.print("w");
}

void button_setup(){
  const byte interruptPin = 2;
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), button_IRQ, CHANGE);
}


void setup() {
   BaseType_t _ret = 1;
  /**
   * Create a queue.
   * https://www.freertos.org/a00116.html
   */
  Serial.begin(115200);  
  charQueue = xQueueCreate(2, sizeof(uint8_t));
  retQueue  = xQueueCreate(1, sizeof(uint8_t));   
  Wire.begin();

  RF_setup();
  MPU_setup();
  button_setup();


  if (charQueue != NULL) {

    // Create task that consumes the queue if it was created.
    _ret = xTaskCreate(TaskSerial, // Task function
                "Ser", // A name just for humans
                300,  // This stack size can be checked & adjusted by reading the Stack Highwater
                NULL, 
                2, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
                NULL);
    if(_ret != pdPASS){
      Serial.println("T1F");
    }
    // Create task that publish data in the queue if it was created.
      _ret = xTaskCreate(TaskReadCommand, // Task function
                "RdCmd", // Task name
                128,  // Stack size
                NULL, 
                2, // Priority
                NULL);    

        if(_ret != pdPASS){
        Serial.println("T1F");
        }
           
  }


  Serial.println("i");
}

void loop() {}


void TaskReadCommand(void *pvParameters)
{
   Serial.println("b");

  (void) pvParameters;
   char sensorValue = 0,retValueQueue;

  for (;;)
  {
     if(LED_state == 0){
 #if 1
     //  Serial.print("L0");
      if (Serial.available()){
        sensorValue = Serial.read();
        xQueueSend(charQueue, &sensorValue, portMAX_DELAY);

        if( xQueueReceive(retQueue, &retValueQueue, portMAX_DELAY) == pdPASS){
        
        }
      }
#endif
     }
     else{
//       Serial.print("L1");
#if 1
        sensorValue = MPU6050_get_incline();
        if(gyro_state == '0' && sensorValue!='0'){
          xQueueSend(charQueue, &sensorValue, portMAX_DELAY);     
            if( xQueueReceive(retQueue, &retValueQueue, portMAX_DELAY) == pdPASS){
        
            }
        }
           
        gyro_state = sensorValue;
#endif
     }
  }
}




/**
 * Serial task.
 * Prints the received items from the queue to the serial monitor.
 */
void TaskSerial(void * pvParameters) {
  (void) pvParameters;
  Serial.println("c");


  uint8_t valueFromQueue = 0;
  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN] = {0};
  uint8_t len = sizeof(buf);

  for (;;) 
  {
   Serial.println("R");
#if 1
     if (xQueueReceive(charQueue, &valueFromQueue, portMAX_DELAY) == pdPASS) {
//        Serial.println(char(valueFromQueue));
        Serial.println("Tx"); //Sending to nrf24_server
        
        nrf24.send(&valueFromQueue, sizeof(valueFromQueue));
        nrf24.waitPacketSent();
        // Now wait for a reply      
        if (nrf24.waitAvailableTimeout(500))
        { 
            // Should be a reply message for us now   
            if (nrf24.recv(buf, &len))
            {
              Serial.print("Rx:");//got reply: 
              Serial.println((char*)buf);
            }
            else
            {
              Serial.println("R5"); //recv failed
            }
        }
        else
        {
            Serial.println("R6");//No reply, is nrf24_server running?"
        }
        xQueueSend(retQueue, &valueFromQueue, portMAX_DELAY); 


    }
    
#endif 
  }
}
