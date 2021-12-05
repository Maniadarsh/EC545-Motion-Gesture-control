// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>

// Include queue support
#include <queue.h>

// Include LedControl library to control the LED matrix
#include <LedControl.h>

// rf
#include <SPI.h>
#include <RH_NRF24.h>

RH_NRF24 nrf24;

#define DISPLAY_DELAY 10
#define NEW_DISPLAY_ANIMATION_SPEED 50
#define AFTER_SPRITE_DELAY 1500

QueueHandle_t charQueue;
LedControl lc = LedControl(6,5,4,1);
TaskHandle_t xHandle = NULL;

void newGame();
void updateDisplay(int delta_x, int delta_y);
void squarePattern();
void patternMatching(int num_expected_input);

int led_x;
int led_y;
int pos[8][8] = {0}; 
int pattern[8][8] = {0};
int init_posY;
int init_posX; 
int num_expected_inputs;
char LED_state = '0';
bool first = true;

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

void loop() {}


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
    if (xQueueReceive(charQueue, &valueFromQueue, 5) == pdPASS){ 
      Serial.print(valueFromQueue);

      if (LED_state == '0'){
         GameOpening();
         LED_state = '1';
      }
      if (LED_state == '1'){ 
        if (valueFromQueue == 's'){
          newGame();
          LED_state = '2';
        }
      }
      else if (LED_state == '2'){
        switch(valueFromQueue){
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
          case '/': // upper right diagonal
            updateDisplay(-1, -1);
            break;
          case '\\': // lower right diagonal
            updateDisplay(1, -1);
            break; 
          case '*': // upper left diagonal
            updateDisplay(-1, 1);
            break;
          case '$': // lower left diagonal
            updateDisplay(1, 1);
            break;
          default:
            break;
       } 
       if (!first){
        if (led_x == init_posX && led_y == init_posY){
          patternMatching(num_expected_inputs);
          LED_state = '1';
        }
        else {
          LED_state = '2';
        }
      }
    }
  }else{
    lc.setLed(0, led_y, led_x,0);
    vTaskDelay( 200 / portTICK_PERIOD_MS );
    lc.setLed(0, led_y, led_x,1);
    vTaskDelay( 200 / portTICK_PERIOD_MS );
  }

 }
}

void squarePattern(){
  Serial.print("\nLet's Draw Square!");
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if (i == 0 || i == 7){
        pattern[i][j] = 1;
        lc.setLed(0, i, j, 1);
      }
      else if (j == 0 || j == 7){
        pattern[i][j] = 1;
        lc.setLed(0, i, j, 1);
      }
    }
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
}

void heartPattern(){
  Serial.print("\nLet's Draw Heart!");
  int count = 0;
  for (int j = 0; j < 8; j++){
    for (int i = 0; i < 8; i++){
        if (j == 1){
          pattern[1][j] = 1;
          pattern[2][j] = 1;
          pattern[5][j] = 1;
          pattern[6][j] = 1;
          lc.setLed(0, 1, j, 1);
          lc.setLed(0, 2, j, 1); 
          lc.setLed(0, 5, j, 1);
          lc.setLed(0, 6, j, 1);
        }
        else if (j == 2 || j == 3 || j == 4){
          if (j == 2){
            pattern[3][j] = 1;
            pattern[4][j] = 1;
            lc.setLed(0, 3, j, 1);
            lc.setLed(0, 4, j, 1); 
          }
          pattern[0][j] = 1;
          pattern[7][j] = 1;
          lc.setLed(0, 0, j, 1);
          lc.setLed(0, 7, j, 1); 
        }
        else if (j == 5){
          pattern[1][j] = 1;
          pattern[6][j] = 1;
          lc.setLed(0, 1, j, 1);
          lc.setLed(0, 6, j, 1); 
        }
        else if (j == 6){
          pattern[2][j] = 1;
          pattern[5][j] = 1;
          lc.setLed(0, 2, j, 1);
          lc.setLed(0, 5, j, 1); 
        }
        else if (j == 7){
          pattern[3][j] = 1;
          pattern[4][j] = 1;
          lc.setLed(0, 3, j, 1);
          lc.setLed(0, 4, j, 1); 
        }
      }
   }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
}

void trianglePattern(){
  Serial.print("\nLet's Draw Triangle!");
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if (j == 7 || j == 7 - i){
        pattern[i][j] = 1;
        lc.setLed(0, i, j, 1); 
      }
      if (i == 7){
        pattern[i][j] = 1;
        lc.setLed(0, i, j, 1);        
      }
    }
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
}

// Function to compute the score of how good the player matches a pattern
// The following is the method of calculation basically using euclidean distance method 
// score = {1 - abs(# of expected inputs - # of inputs)/(# of expected inputs)} * {1 - (expected value in the pattern vector - value in the position vector))/# of array} * 100
void patternMatching(int num_expected_inputs){
  float num_diff = 0;
  float score = 0;
  float num_input = 0;

  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if (pattern[i][j] != pos[i][j]){
        num_diff++;
      }
      if (pos[i][j] == 1){
        num_input++;
      }
    }
  }

  score = (1 - abs(num_expected_inputs - num_input)/num_expected_inputs)*((64 - num_diff)/64)*100;
  
  if (num_diff == 0){
    Serial.println("\nSuccess! Good Job!");
    setSprite(circle);
  }
  else {
    char buffer[30];
    int matching_score = score;
    sprintf(buffer, "\nFail! The Matching Score Is %d", matching_score);
    Serial.println(buffer);
    setSprite(cross);
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
  memset(pos, 0, sizeof(pos));
  vTaskSuspend( xHandle );
}

void setSprite(byte *sprite){
    for(int r = 0; r < 8; r++){
        lc.setRow(0, r, sprite[r]);
    }
}

void setInitPosition(int y, int x){
   lc.setLed(0, y, x, 1);
   init_posY = y;
   init_posX = x;
   led_y = y;
   led_x = x; 
}

void updateDisplay(int delta_x, int delta_y){
  lc.setLed(0, led_y, led_x,1);
  if ((led_x + delta_x <= 7 && led_x + delta_x >= 0) && (led_y + delta_y <= 7 && led_y + delta_y >= 0)){ 
    led_y += delta_y;
    led_x += delta_x;
  }
  else if ((led_x + delta_x > 7 || led_x + delta_x < 0) && (led_y + delta_y <= 7 && led_y + delta_y >= 0)){
    if (delta_x == 0 || delta_y == 0){ // if not diagonal movement 
      led_y += delta_y;
    }
  }
  else if ((led_x + delta_x <= 7 && led_x + delta_x >= 0) && (led_y + delta_y > 7 || led_y + delta_y < 0)){
    if (delta_x == 0 || delta_y == 0){ // if not diagonal movement 
      led_x += delta_x;
    }
  }
  lc.setLed(0, led_y, led_x,1);
  pos[led_y][led_x] = 1;
  first = false;
}

void GameOpening(){
  lc.clearDisplay(0);
  Serial.print("\nNew Game Is Started!");   
  for(int r = 0; r < 8; r++){
    for(int c = 0; c < 8; c++){
      lc.setLed(0, r, c, HIGH);
      delay(NEW_DISPLAY_ANIMATION_SPEED);
    }
  }
  lc.clearDisplay(0);
}

void newGame() {
    memset(pos, 0, sizeof(pos));
    first = true; 
    int pattern = rand()%3+1;
    switch(pattern){
      case 1:
        squarePattern();
        setInitPosition(0,0);
        num_expected_inputs = 28;
        break;
      case 2: 
        trianglePattern();
        setInitPosition(7,7);
        num_expected_inputs = 21;
        break;
      case 3:
        heartPattern();
        setInitPosition(4,7);
        num_expected_inputs = 18;
        break;
    }
    vTaskResume( xHandle );
}
 
/* On Board Blink */
void TaskBlink(void *pvParameters)
{
  (void) pvParameters;

  for (;;)
  {
    lc.setLed(0, led_y, led_x,0);
    vTaskDelay( 200 / portTICK_PERIOD_MS );
    lc.setLed(0, led_y, led_x,1);
    vTaskDelay( 200 / portTICK_PERIOD_MS );
  }
}

void TaskReadCommand(void *pvParameters)
{
  (void) pvParameters;
  
  for (;;)
  {
    // Read the input on analog pin 0:
    char sensorValue = 0;
    if (nrf24.available())
    {
      // Should be a message for us now   
      uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if (nrf24.recv(buf, &len))
      {
  //      NRF24::printBuffer("request: ", buf, len);
        Serial.print("got : ");
        Serial.println(char(buf[0]));
        sensorValue = buf[0];
        // Send a reply
        uint8_t data[] = "Y";
        nrf24.send(data, sizeof(data));
        nrf24.waitPacketSent();
        Serial.println("Sent a reply");
      }
      else
      {
        sensorValue = 'N';
        Serial.println("recv failed");
      }
    }
    xQueueSend(charQueue, &sensorValue, portMAX_DELAY);

    // One tick delay (15ms) in between reads for stability
    vTaskDelay(1);
  }
}

void TaskSerialCommand(void *pvParameters)
{
  (void) pvParameters;
  while (!Serial) {
    vTaskDelay(1);
  }
  for (;;)
  {
     char sensorValue = 0;
     if (Serial.available()){
        sensorValue = Serial.read();
        xQueueSend(charQueue, &sensorValue, portMAX_DELAY);
        Serial.println("cmd received");
     }
     vTaskDelay(1);
  }
}

void setup() {
  lc.shutdown(0,false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  /**
   * Create a queue.
   * https://www.freertos.org/a00116.html
   */
  charQueue = xQueueCreate(10, // Queue length
                              sizeof(char) // Queue item size
                              );
  
  if (charQueue != NULL) {
    
    // Create task that consumes the queue if it was created.
    xTaskCreate(TaskDisplay, // Task function
                "Display", // A name just for humans
                128,  // This stack size can be checked & adjusted by reading the Stack Highwater
                NULL, 
                2, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
                NULL);


    // Create task that publish data in the queue if it was created.
    xTaskCreate(TaskReadCommand, // Task function
                "ReadCommand", // Task name
                256,  // Stack size
                NULL, 
                2, // Priority
                NULL);
/*
    xTaskCreate(TaskSerialCommand, // Task function
                    "ReadSCommand", // Task name
                    128,  // Stack size
                    NULL, 
                    2, // Priority
                    NULL);

    xTaskCreate(TaskBlink, // Task function
              "Blink", // Task name
              128, // Stack size 
              NULL, 
              0, // Priority
              &xHandle);*/

  }
              
  Serial.begin(9600);
  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");
  Serial.println("init end");
}
