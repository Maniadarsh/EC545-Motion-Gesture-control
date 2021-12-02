// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>

// Include queue support
#include <queue.h>

// Include LedControl library to control the LED matrix
#include <LedControl.h>

#define DISPLAY_DELAY 10
#define NEW_DISPLAY_ANIMATION_SPEED 50
#define AFTER_SPRITE_DELAY 1500

QueueHandle_t charQueue;
LedControl lc = LedControl(7,6,5,1);

void newDisplay();
void errorDisplay();
void stopDisplay();
void updateDisplay(int delta_x, int delta_y);
void squarePattern();
void patternMatching(int num_expected_input);

int led_x = 0;
int led_y = 0;
int pos[8][8] = {0}; 
int pattern1[8][8] = {0};

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
                128,  // Stack size
                NULL, 
                2, // Priority
                NULL);

    xTaskCreate(TaskBlink, // Task function
              "Blink", // Task name
              128, // Stack size 
              NULL, 
              0, // Priority
              &xHandle);

  }
              
  Serial.begin(9600);

}

void loop() {}

void TaskReadCommand(void *pvParameters)
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
     }
     vTaskDelay(1);
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
    if (xQueueReceive(charQueue, &valueFromQueue, portMAX_DELAY) == pdPASS) {
      Serial.print(valueFromQueue);
      switch(valueFromQueue) {
        case 'n':
          newDisplay();
          // squarePattern();
          setInitPosition();
          break;
        case 'e':
          errorDisplay();
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
          break;
      }
    }
  }
}

void squarePattern(){
  Serial.print("\nLet's Draw Square!");
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
    Serial.println("\nSuccess! Good Job!");
    setSprite(circle);
  }
  else {
    char buffer[30];
    int matching_score = score;
    sprintf(buffer, "\nFail! The Matching Score Is %d", matching_score);
    Serial.println(buffer);
    delete[] buffer;
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
    Serial.print("\nNew Game Is Started!");   
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
    Serial.print("\nSome Error is Occurred!");
    setSprite(cross);
    delay(AFTER_SPRITE_DELAY);
    lc.clearDisplay(0);
    lc.setLed(0, led_y, led_x, 1);
}
 
void stopDisplay() {
    lc.clearDisplay(0);
    Serial.print("\nGame is terminated!");
    for(int r = 0; r < 8; r++){
        for(int c = 0; c < 8; c++){
            lc.setLed(0, r, c, HIGH);
            delay(NEW_DISPLAY_ANIMATION_SPEED);
        }
    }
    lc.clearDisplay(0);
    vTaskSuspend(xHandle);
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
