# EC545-Motion-Gesture-Control
Cyberphysical systems project repository.
Group Name: MGC

# Introduction
Our Topic: Motion Gesture Control (abbrevated as MGC) is a trend of Human Machine Interface (HMI). It is a good demonstration of cyber-physical system because it translate the movement of object in physical world into digital signal. Our project aims at prototyping of a wireless hand-motion capturing system. We will prove our concept by a interactive game.  

# Hardware Deployment
* Arduino #1: Control
  * Accelerometer 
  * Wireless Module 
* Arduino #2: Display
  * LED matrix
  * Wireless Module

# Demonstration Usage

The user holds the Control Arduino in her/his hand. Upon power-on, The LED matrix will show a welcome message. After that, the user should be ready to start the game. After the user press a button, the LED matrix will show a pattern, a start point, and an end point on the matrix. A blinking cursor indicated the current position of the user. The user should control the cursor and make it follow the pattern to the end point. The cursor will move in the same direction as the user wave her/his hand. When the cursor reaches the end point, the LED will show PASS or FAIL of the trace made by cursor. 

# Message Sequence between components





# State Transition


