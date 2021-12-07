// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <LedControl.h>

// rf
#include <SPI.h>
#include <RH_NRF24.h>

RH_NRF24 nrf24;
QueueHandle_t charQueue;
QueueHandle_t retQueue;

#define DISPLAY_DELAY 10
#define NEW_DISPLAY_ANIMATION_SPEED 50
#define AFTER_SPRITE_DELAY 1500
LedControl lc = LedControl(6,5,4,1);

//****************************
void newDisplay();
void updateDisplay(int delta_x, int delta_y);
void patternMatching(int num_expected_inputs);
void gameOpening();

char led_x = 0;
char led_y = 0;
bool pos[8][8] = {0}; 
bool pattern[8][8] = {0};
int init_posY;
int init_posX; 
int num_expected_inputs;
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

char LED_state;


void LED_matrix_setup(){

  lc.shutdown(0,false);
  lc.setIntensity(0, 0);
  lc.clearDisplay(0);
  LED_state = 0;

  charQueue = xQueueCreate(3, // Queue length
                              sizeof(char) // Queue item size
                              );
  retQueue = xQueueCreate(1,sizeof(char)); 

}

void RF_setup(){

  if (!nrf24.init()){
    Serial.println("rfInitFail");
  }
  if (!nrf24.setChannel(1)){
    Serial.println("setChnlFail");
  }
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm)){
    Serial.println("setRfFailed");    
  }
  Serial.println("initEnd");

}


void setup() {

  Serial.begin(115200);
  LED_matrix_setup();
  RF_setup();
  
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
                300,  // Stack size
                NULL, 
                2, // Priority
                NULL);

  }
              

  
}





void loop() {}
 
void TaskReadCommand(void *pvParameters)
{

    uint8_t sensorValue = 0;
    uint8_t valueFromQueue;
    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN] = {0};
    uint8_t len = sizeof(buf);
  (void) pvParameters;
  for (;;)
  {

    sensorValue = 0;

    if (nrf24.available())
    {
      if (nrf24.recv(buf, &len))
      {
        Serial.print("Rx:");
        Serial.println((char*)buf);
        sensorValue = buf[0];

      }
      else
      {
        sensorValue = 'N';
        Serial.println("RN");//recv failed
      }
    }
    if(sensorValue!=0){
      xQueueSend(charQueue, &sensorValue, portMAX_DELAY);
      Serial.println("TQQ");
      if(xQueueReceive(retQueue, &valueFromQueue, portMAX_DELAY) == pdPASS){
        //Serial.println("RP");//Sent a reply
        nrf24.send(buf, sizeof(buf));
        nrf24.waitPacketSent();
      }
    }

  }
}

void TaskDisplay(void * pvParameters) {
  (void) pvParameters;

  char valueFromQueue;

  for (;;) 
  {
#if 1    
    /**
     * Read an item from a queue.
     * https://www.freertos.org/a00118.html
     */
    if (LED_state == 0){  
        LED_state = 1;//??
     } 
    else if (xQueueReceive(charQueue, &valueFromQueue, 5) == pdPASS) {
      Serial.print("Q:");
      Serial.print(valueFromQueue);
      if (LED_state == 1){  
        newGame();
        LED_state = 2;
      }
  
      else if (LED_state == 2){
        switch(valueFromQueue) {
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
      }
      if (!first){
        if (led_y == init_posY && led_x == init_posX){
          patternMatching(num_expected_inputs);
          LED_state = 1;//'1'
        }
      }

      xQueueSend(retQueue, &valueFromQueue, portMAX_DELAY); 
    }
    else{
      lc.setLed(0, led_y, led_x,0);
      vTaskDelay( 200 / portTICK_PERIOD_MS );
      lc.setLed(0, led_y, led_x,1);
      vTaskDelay( 200 / portTICK_PERIOD_MS );
    }
#endif
  }
}

void squarePattern(){
  Serial.print("\nLet's Draw Square!\n");
  int count = 0;
  int score = 0;
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
  Serial.print("\nLet's Draw Heart!\n");
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
          //lc.setLed(0, 3, j, 1);
          //lc.setLed(0, 4, j, 1); 
        }
      }
   }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
}

void trianglePattern(){
  Serial.print("\nLet's Draw Triangle!\n");
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
    Serial.println("\nSuccess!");
    setSprite(circle);
  }
  else {
    char buffer[30];
    int matching_score = score;
    sprintf(buffer, "\nScore:%d", matching_score);
    Serial.println(buffer);
    setSprite(cross);
  }
  delay(AFTER_SPRITE_DELAY);
  lc.clearDisplay(0);
  memset(pos, 0, sizeof(pos));
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
  first = false;
}

void gameOpening(){  
  lc.clearDisplay(0); 
  for(int r = 0; r < 8; r++){
    for(int c = 0; c < 8; c++){
      lc.setLed(0, r, c, HIGH);
      delay(NEW_DISPLAY_ANIMATION_SPEED);
    }
  }
  lc.clearDisplay(0);
}

void setInitPosition(int y, int x){
   Serial.print("\nNew Game Is Started!");  
   lc.setLed(0, y, x, 1);
   init_posY = y;
   init_posX = x;
   led_y = y;
   led_x = x; 
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
        //squarePattern();
        trianglePattern();
        setInitPosition(7,7);
        num_expected_inputs = 21;
        break;
      case 3:
        //squarePattern();
        heartPattern();
        setInitPosition(4,7);
        num_expected_inputs = 18;
        break;
    }
}
