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
        
        // Now wait for a reply
        // retry 2 times every 200 ms, could change these value to get it better
        // 1) msg lost, need to resend the message
        // 2) transmission not complete, using bigger timeout interval
        // 3) waiting too long, reduce the timeout interval
        
        for(int i=0;i<3;i++){                              // retry time is here
          nrf24.send(&valueFromQueue, sizeof(valueFromQueue));
          nrf24.waitPacketSent();
          if (nrf24.waitAvailableTimeout(200))             // timeout interval is here
          { 
              // Should be a reply message for us now   
              if (nrf24.recv(buf, &len))
              {
                Serial.print("Rx:");//got reply: 
                Serial.println((char*)buf);
                break;
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
        }      
        xQueueSend(retQueue, &valueFromQueue, portMAX_DELAY); 
    } 
#endif 
  }
}