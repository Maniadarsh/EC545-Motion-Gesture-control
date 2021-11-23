# EC545-Motion-Gesture-Control
Cyberphysical systems project repository.
Group Name: MGC

# Introduction
Our Topic: Motion Gesture Control (abbrevated as MGC) is a trend of Human Machine Interface (HMI). It is a good demonstration of cyber-physical system because it translate the movement of object in physical world into digital signal. Our project aims at prototyping of a wireless hand-motion capturing system. We will prove our concept by a interactive game.  

# Hardware Deployment [Reference](https://github.com/Maniadarsh/EC545-Motion-Gesture-control/wiki#datasheet-and-example) 
* Arduino #1: Control
  * Accelerometer 
  * Wireless Module 
* Arduino #2: Display
  * LED matrix
  * Wireless Module

# Demonstration Usage

The user holds the Control Arduino in her/his hand. Upon power-on, The LED matrix will show a welcome message. After that, the user should be ready to start the game. After the user press a button, the LED matrix will show a pattern, a start point, and an end point on the matrix. A blinking cursor indicated the current position of the user. The user should control the cursor and make it follow the pattern to the end point. The cursor will move in the same direction as the user wave her/his hand. When the cursor reaches the end point, the LED will show PASS or FAIL of the trace made by cursor. 

# Message Sequence between components

![Sequence Diagram](https://github.com/Maniadarsh/EC545-Motion-Gesture-control/blob/main/pictures/SequenceDiagram.png)
[Link to Edit](https://lucid.app/lucidchart/36681cb7-d538-4060-a365-803737bf8723/edit?viewport_loc=144%2C254%2C1732%2C822%2C0_0&invitationId=inv_58f1c76f-a07f-45b2-a2c0-2ab4e1ce7ecc)


# State Transition

# Why we use FreeRTOS

FreeRTOS is the most popular Real-time Operating System for embedded system. It uses a fixed-priority preemptive scheduling policy, with round-robin time-slicing of equal priority tasks ([FreeRTOS](https://freertos.org/index.html)). Each task in the operating system can work independently, while the scheduler ensures CPU resource will not be occupied by a single function for a long time. In our application, multiple tasks and components works at the same time, and they all have different scales of execution time, that being said we cannot simply put everything in one single while loop. The best way to make them orchestrate smoothly is to use FreeRTOS. Besides, the Arduino community has abundant support and discussion in FreeRTOS, making related development easy.

## Tasks Description 
### Arduino #1 Control Tasks
* Accelerometer Driver: Decide the current direction of motion 
* RF Transmitter: Send Command through RF module
* Status LED Blinker: Show current state on on-board LED by changing blinking frequency
* Serial Port: Send debug message through UART

### Arduino #2 Display Tasks
* LED Matrix: Control LED matrix display and host the game scoring system
* RF Receiver: Receive command from RF module
* Status LED Blinker: Show current state on on-board LED by changing blinking frequency
* Serial Port: Send debug message through UART


