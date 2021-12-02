/*
 * Example of a basic FreeRTOS queue
 * https://www.freertos.org/Embedded-RTOS-Queues.html
 */

// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>

// Include queue support
#include <queue.h>
#include <SPI.h>
#include <RH_NRF24.h>
/* 
 * Declaring a global variable of type QueueHandle_t 
 * 
 */
QueueHandle_t integerQueue;
RH_NRF24 nrf24;

void RF_setup() 
{
  Serial.begin(9600);
  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");    
}

void setup() {

  /**
   * Create a queue.
   * https://www.freertos.org/a00116.html
   */
  integerQueue = xQueueCreate(10, // Queue length
                              sizeof(uint8_t) // Queue item size
                              );
  RF_setup();

  if (integerQueue != NULL) {
    
    // Create task that consumes the queue if it was created.
    xTaskCreate(TaskSerial, // Task function
                "Serial", // A name just for humans
                256,  // This stack size can be checked & adjusted by reading the Stack Highwater
                NULL, 
                1, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
                NULL);


    // Create task that publish data in the queue if it was created.
    xTaskCreate(TaskAnalogRead, // Task function
                "AnalogRead", // Task name
                128,  // Stack size
                NULL, 
                1, // Priority
                NULL);
    
  }


}

void loop() {}


/**
 * Analog read task
 * Reads an analog input on pin 0 and send the readed value through the queue.
 * See Blink_AnalogRead example.
 */
void TaskAnalogRead(void *pvParameters)
{
  (void) pvParameters;
  
  for (;;)
  {
    // Read the input on analog pin 0:
    uint8_t sensorValue = 0;

    /**
     * Post an item on a queue.
     * https://www.freertos.org/a00117.html
     */

    if (nrf24.available())
    {
      // Should be a message for us now   
      uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if (nrf24.recv(buf, &len))
      {
  //      NRF24::printBuffer("request: ", buf, len);
        Serial.print("got request: ");
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
    xQueueSend(integerQueue, &sensorValue, portMAX_DELAY);

    // One tick delay (15ms) in between reads for stability
    vTaskDelay(100);
  }
}

/**
 * Serial task.
 * Prints the received items from the queue to the serial monitor.
 */
void TaskSerial(void * pvParameters) {
  (void) pvParameters;

  uint8_t valueFromQueue = 0;

  for (;;) 
  {
 
    /**
     * Read an item from a queue.
     * https://www.freertos.org/a00118.html
     */
    
    if (xQueueReceive(integerQueue, &valueFromQueue, portMAX_DELAY) == pdPASS) {
      Serial.println(char(valueFromQueue));
    }
  }
}
