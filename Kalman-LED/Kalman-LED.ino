// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>

// Include queue support
#include <queue.h>

// Include LedControl library to control the LED matrix
#include <LedControl.h>
#include <Wire.h>
#include <Kalman.h> // Source: https://github.com/TKJElectronics/KalmanFilter
#define MPU_ADDR  0x68

#define DISPLAY_DELAY 10
#define NEW_DISPLAY_ANIMATION_SPEED 50
#define AFTER_SPRITE_DELAY 1500
#define RESTRICT_PITCH
Kalman kalmanX; // Create the Kalman instances
Kalman kalmanY;

QueueHandle_t charQueue;
QueueHandle_t retQueue;
LedControl lc = LedControl(12,11,10,1);

/* IMU Data */
double accX, accY, accZ;
double gyroX, gyroY, gyroZ;
int16_t tempRaw;

double gyroXangle, gyroYangle; // Angle calculate using the gyro only
double compAngleX, compAngleY; // Calculated angle using a complementary filter
double kalAngleX, kalAngleY; // Calculated angle using a Kalman filter

uint32_t timer;
uint8_t i2cData[14]; // Buffer for I2C data
char gyro_state;
// TODO: Make calibration routine

/* I2C setup */

//#define W_THRESHOLD  15
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
#if 0
  Serial.print(roll); Serial.print("\t");
  //Serial.print(gyroXangle); Serial.print("\t");
  //Serial.print(compAngleX); Serial.print("\t");
  Serial.print(kalAngleX); Serial.print("\t");

  Serial.print("\t");

  Serial.print(pitch); Serial.print("\t");
  //Serial.print(gyroYangle); Serial.print("\t");
  //Serial.print(compAngleY); Serial.print("\t");
  Serial.print(kalAngleY); Serial.print("\t");
#endif 
#if 0
  char direct;
  direct = directions(kalAngleX,kalAngleY);
  
    
  Serial.print(direct);
  Serial.print("\r\n");
#endif
  return directions(kalAngleX,kalAngleY);
}


char directions(double kalAngleX,double kalAngleY){
  char dir = '0';
  int8_t threshold = 15;
  bool xp = (kalAngleX > threshold); 
  bool xn = (kalAngleX < -threshold); 
  bool xz = (kalAngleX < threshold) && (kalAngleX > -threshold);
  bool yp = (kalAngleY > threshold); 
  bool yn = (kalAngleY < -threshold); 
  bool yz = (kalAngleY < threshold) && (kalAngleY > -threshold);
  if(xp && yz) dir = 'r';
  if(xn && yz) dir = 'l';
  if(xp && yp) dir = '/';
  if(xn && yp) dir = '\\';
  if(xp && yn) dir = '*';
  if(xn && yn) dir = '$';
  if(xz && yp) dir = 'u';
  if(xz && yn) dir = 'd';
  return dir;
}



//****************************
void newDisplay();
void errorDisplay();
void stopDisplay();
void updateDisplay(int delta_x, int delta_y);
void squarePattern();
void patternMatching(int num_expected_input);

char led_x = 0;
char led_y = 0;
bool pos[8][8] = {0}; 
bool pattern1[8][8] = {0};

byte cross[] = {
B10000001,
B01000010,
B00100100,
B00011000,
B00011000,
B00100100,
B01000010,
B10000001
};
 
byte circle[] = {
B00111100,
B01000010,
B10000001,
B10000001,
B10000001,
B10000001,
B01000010,
B00111100
};

TaskHandle_t xHandle = NULL;
char LED_state;


void LED_matrix_setup(){
  
  lc.shutdown(0,false);
  lc.setIntensity(0, 0);
  lc.clearDisplay(0);
  LED_state = 0;

  charQueue = xQueueCreate(6, // Queue length
                              sizeof(char) // Queue item size
                              );
  retQueue = xQueueCreate(1,sizeof(char)); 
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
    Serial.print(F("Error reading sensor"));
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
}
void button_setup(){
  const byte interruptPin = 2;
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), button_IRQ, CHANGE);
}
void setup() {

  /**
   * Create a queue.
   * https://www.freertos.org/a00116.html
   */
  Serial.begin(115200);
  LED_matrix_setup();
  MPU_setup();
  button_setup();
  Wire.begin();

  
  if (charQueue != NULL) {
    
    // Create task that consumes the queue if it was created.
    xTaskCreate(TaskDisplay, // Task function
                "Dpl", // A name just for humans
                128,  // This stack size can be checked & adjusted by reading the Stack Highwater
                NULL, 
                2, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
                NULL);


    // Create task that publish data in the queue if it was created.
    xTaskCreate(TaskReadCommand, // Task function
                "RdCmd", // Task name
                128,  // Stack size
                NULL, 
                2, // Priority
                NULL);
#if 0
    xTaskCreate(TaskBlink, // Task function
              "Bnk", // Task name
              32, // Stack size 
              NULL, 
              0, // Priority
              &xHandle);
#endif
  }
             
  
}

void loop() { }



void TaskReadCommand(void *pvParameters)
{
  (void) pvParameters;
   char sensorValue = 0,retValueQueue;
   TickType_t xLastWakeTime;
  while (!Serial) {
    vTaskDelay(1);
  }
  
  for (;;)
  {
 //   xLastWakeTime = xTaskGetTickCount();
     if(LED_state == 0){
      if (Serial.available()){
        sensorValue = Serial.read();
        xQueueSend(charQueue, &sensorValue, portMAX_DELAY);
       // Serial.println(sensorValue);
        if( xQueueReceive(retQueue, &retValueQueue, portMAX_DELAY) == pdPASS){
        
        }
      }

     }
     else{
        sensorValue = MPU6050_get_incline();
        if(gyro_state == '0' && sensorValue!='0'){
          xQueueSend(charQueue, &sensorValue, portMAX_DELAY);     
            if( xQueueReceive(retQueue, &retValueQueue, portMAX_DELAY) == pdPASS){
        
            }
        }
//        if(gyro_state == '?'){
//          
//        }
           
        gyro_state = sensorValue;
     }
     
//    vTaskDelayUntil( &xLastWakeTime, 333/portTICK_PERIOD_MS );
  }
}

void TaskDisplay(void * pvParameters) {
  (void) pvParameters;

  // Wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  while (!Serial) {
    vTaskDelay(1);
  }

  char valueFromQueue;

  for (;;) 
  {
    /**
     * Read an item from a queue.
     * https://www.freertos.org/a00118.html
     */
    if (xQueueReceive(charQueue, &valueFromQueue, 5) == pdPASS) {
     // Serial.print(valueFromQueue);
      switch(valueFromQueue) {
        case 'n':
          newDisplay();directions(kalAngleX,kalAngleY);
          // squarePattern();
          xQueueReset(charQueue);
          setInitPosition();

          break;
        case 'e':
          errorDisplay();
          xQueueReset(charQueue);
          break;
        case 's':
          stopDisplay();
          break;
        case 'u': 
          updateDisplay(-1, 0);
          break;
        case 'd':
          updateDisplay(1, 0);
          break;
        case 'r':
          updateDisplay(0, -1);
          break;
        case 'l':
          updateDisplay(0, 1);
          break;
        case 'm':
          patternMatching(28);
          xQueueReset(charQueue);
          break;
        case 'G':
          LED_state =1 ;          
          newDisplay();
          setInitPosition();
          xQueueReset(charQueue);
        default:
         break;
      }
      xQueueSend(retQueue, &valueFromQueue, portMAX_DELAY); 
    }
    else{
      lc.setLed(0, led_y, led_x,0);
      vTaskDelay( 200 / portTICK_PERIOD_MS );
      lc.setLed(0, led_y, led_x,1);
      vTaskDelay( 200 / portTICK_PERIOD_MS );
    }

  }
}

void squarePattern(){
  Serial.print("\nLet's Square!");
  int count = 0;
  int score = 0;
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if (i == 0 || i == 7){
        pattern1[i][j] = 1;
        lc.setLed(0, i, j, 1);
      }
      else if (j == 0 || j == 7){
        pattern1[i][j] = 1;
        lc.setLed(0, i, j, 1);
      }
    }
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
}

/*
void trianglePattern(){
  Serial.print("\nHere Comes...Triangle!");
  int count = 0;
  int score = 0;
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if ((i == j) || (i == j + 1)){
        pattern1[i][j] = 1;
        lc.setLed(0, i, j, 1);
        }
      if (i == 0){
        pattern1[i][j] = 1;
        lc.setLed(0, i, j, 1);
      }
      if (j == 7){
        pattern1[i][j] = 1;
        lc.setLed(0, i, j, 1);
      }
    }
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
}*/

// Function to compute the score of how good the player matches a pattern
// The following is the method of calculation basically using euclidean distance method 
// score = {1 - abs(# of expected inputs - # of inputs)/(# of expected inputs)} * {1 - (expected value in the pattern vector - value in the position vector))/# of array} * 100
void patternMatching(int num_expected_inputs){
  float num_diff = 0;
  float score = 0;
  float num_input = 0;

  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if (pattern1[i][j] != pos[i][j]){
        num_diff++;
      }
      if (pos[i][j] == 1){
        num_input++;
      }
    }
  }

  score = (1 - abs(num_expected_inputs - num_input)/num_expected_inputs)*((64 - num_diff)/64)*100;
  
  if (num_diff == 0){
    Serial.println("\nSuccess!");
    setSprite(circle);
  }
  else {
    char buffer[30];
    int matching_score = score;
    sprintf(buffer, "\nScore:%d", matching_score);
    Serial.println(buffer);
   // delete[] buffer;
    setSprite(cross);
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
  memset(pos, 0, sizeof(pos));
  setInitPosition();
}

void setSprite(byte *sprite){
    for(int r = 0; r < 8; r++){
        lc.setRow(0, r, sprite[r]);
    }
}

void updateDisplay(int delta_x, int delta_y){
  lc.setLed(0, led_y, led_x,1);
  if ((led_x + delta_x <= 7 && led_x + delta_x >= 0) && (led_y + delta_y <= 7 && led_y + delta_y >= 0)){ 
    led_y += delta_y;
    led_x += delta_x;
  }
  else if ((led_x + delta_x > 7 || led_x + delta_x < 0) && (led_y + delta_y <= 7 && led_y + delta_y >= 0)){
    led_y += delta_y;
  }
  else if ((led_x + delta_x <= 7 && led_x + delta_x >= 0) && (led_y + delta_y > 7 || led_y + delta_y < 0)){
    led_x += delta_x;
  }
  lc.setLed(0, led_y, led_x,1);
  pos[led_y][led_x] = 1;
  if (led_x == 0 && led_y == 0){
    patternMatching(28);
  }
}

void setInitPosition(){
   lc.setLed(0, 0, 0, 1);
   led_y = 0; 
   led_x = 0;
}
 
void newDisplay() {
    lc.clearDisplay(0);
    Serial.print("\nNew Game");   
    for(int r = 0; r < 8; r++){
        for(int c = 0; c < 8; c++){
            lc.setLed(0, r, c, HIGH);
            delay(NEW_DISPLAY_ANIMATION_SPEED);
        }
    }
    lc.clearDisplay(0);
    memset(pos, 0, sizeof(pos));
    vTaskResume( xHandle );

    int pattern = rand() % 3 + 1;
    switch(pattern){
      case 1:
        squarePattern();
        break;
      case 2:
        squarePattern();
        break;
      case 3: 
        squarePattern();
        break;
    }
}

void errorDisplay() {
    lc.clearDisplay(0);
    Serial.print("\nError!");
    setSprite(cross);
    delay(AFTER_SPRITE_DELAY);
    lc.clearDisplay(0);
    lc.setLed(0, led_y, led_x, 1);
}
 
void stopDisplay() {
    lc.clearDisplay(0);
    Serial.print("\nTerminated");
    for(int r = 0; r < 8; r++){
        for(int c = 0; c < 8; c++){
            lc.setLed(0, r, c, HIGH);
            delay(NEW_DISPLAY_ANIMATION_SPEED);
        }
    }
    lc.clearDisplay(0);
    vTaskSuspend(xHandle);
}

#if 1
/* On Board Blink */
void TaskBlink(void *pvParameters)
{
  for (;;)
  {

    lc.setLed(0, led_y, led_x,0);
    vTaskDelay( 200 / portTICK_PERIOD_MS );
    lc.setLed(0, led_y, led_x,1);
    vTaskDelay( 200 / portTICK_PERIOD_MS );
  }
}

#endif
