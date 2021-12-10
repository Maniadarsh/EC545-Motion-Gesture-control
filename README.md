# EC545-Motion-Gesture-Control
Cyberphysical systems project repository.
Group Name=Product Name= ?

# Introduction and Motivation
Our Topic: Motion Gesture Control (abbrevated as MGC) is a trend of Human Machine Interface (HMI). It is a good demonstration of cyber-physical system because it translate the movement of object in physical world into digital signal. Our project aims at prototyping of a wireless hand-motion capturing system. We will prove our concept by a interactive game. Our software will run on Arduino Uno board with FreeRTOS

[Proposal Slide](https://docs.google.com/presentation/d/1rdGEFLtZfjV_AjMn_7U8uSxR3QpZAKa-A2tnY07p7gk/edit?usp=sharing)

[Progress Update Slide 11/29](https://docs.google.com/presentation/d/1_m6CIY_wS7HHzbtLpxGFxYQQdaCvxSt0freQKTb6WAY/edit#slide=id.p)
# Hardware Deployment [Reference](https://github.com/Maniadarsh/EC545-Motion-Gesture-control/wiki#datasheet-and-example) 
* Arduino #1: Control
  * Accelerometer 
  * Wireless Module 
* Arduino #2: Display
  * LED matrix
  * Wireless Module

# Demonstration Usage

The user holds the Control Arduino in her/his hand. Upon power-on, The LED matrix will show a welcome message. After that, the user should be ready to start the game. After the user press a button, the LED matrix will show a pattern, a start point, and an end point on the matrix. A blinking cursor indicated the current position of the user. The user should control the cursor and make it follow the pattern to the end point. The cursor will move in the same direction as the user wave her/his hand. When the cursor reaches the end point, the LED will show PASS or FAIL of the trace made by cursor. 

# System Description

| Current State |  input | | Next State |  Output |
|---------------|-------------|-|------------|---------------|
| -->IDLE          | Start Button||Game ON     | Pattern, Cursor |
|Game ON        | Human Movement    | | Continue   | New Cursor    |
|Continue       | TimeUp     | | Finising  |   Score|

## Subsystem 
Tx(.) means the transmission of RF module.  Rx(.) means the reception of RF module.

* Arduino #1

|Current State | input  || Next State | Output | 
|-------------|-------|-|------|-------|
|--> Startup |    x     ||  Startup     |   LED Blinking Slow  |
|  Startup          |    Start Button   ||  Receiving Motion    | LED Blinking Fast     |
|  Receiving Motion |    Human Motion   ||  Receiving Motion    | LED Blinking Fast, Tx(Encoded message)       |
|  Receiving Motion |   Rx(Termination)    ||  Resting     | LED ON      |
|Resting | Start Button || Startup |    LED Blinking Slow    | 


* Arduino #2

|Current State | input  || Next State | Output | 
|-------------|-------|-|------|-------|
|--> Startup |    x     ||  Startup     |   Startup Animation  |
|  Startup |    Rx(S)   ||  Game Start    |   New Pattern, Cursor   |
|Game Start | Rx(U,D,R,L)  || Game Start  |   New Cursor, Path      |
|Game Start | Back to Starting point || Good Game |  Score   |
|Good Game |    Rx(S)     ||  Game Start       |  New Pattern Cursor      |

[State Diagram](https://app.diagrams.net/#G1d1R1qwncyxGttj_RDuuZV8x_IldtKNCK)  
[Overviw](https://drive.google.com/file/d/1EMjD77yjHDF4gCPTlry1tbx8tKwp95wu/view?usp=sharing)

# Message Sequence between components

![Sequence Diagram](https://github.com/Maniadarsh/EC545-Motion-Gesture-control/blob/main/pictures/SequenceDiagram.png)
[Link to Edit](https://lucid.app/lucidchart/36681cb7-d538-4060-a365-803737bf8723/edit?viewport_loc=144%2C254%2C1732%2C822%2C0_0&invitationId=inv_58f1c76f-a07f-45b2-a2c0-2ab4e1ce7ecc)


# State Transition

# Why we use FreeRTOS

FreeRTOS is the most popular Real-time Operating System for embedded system. It uses a fixed-priority preemptive scheduling policy, with round-robin time-slicing of equal priority tasks ([FreeRTOS website](https://freertos.org/index.html)). Each task in the operating system can work independently, while the scheduler ensures CPU resource will not be occupied by a single function for a long time. In our application, multiple tasks and components works at the same time, and they all have different scales of execution time, that being said we cannot simply put everything in one single while loop. The best way to make them orchestrate smoothly is to use FreeRTOS. Besides, the Arduino community has abundant support and discussion in FreeRTOS, making related development easy.

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


# Timeilne
![Timeline](https://github.com/Maniadarsh/EC545-Motion-Gesture-control/blob/main/pictures/Timeline.png)
# Job Assignment
|         | Hardware | LED Matrix | RF Module | Accelerometer | FreeRTOS | Arduino #1 | Arduino #2 |
|--       |----------|------------|-----------|---------------|----------|------------|------------|
|Mani     |          |            |           |       X       |          |     X      |            |
|Ruei-Yang|   X      |            |           |               |    X     |            |     X      |
|Tianhe   |          |            |    X      |       X       |          |     X      |            |
|Yuko     |          |     X      |           |               |    X     |            |     X      |


# TODO 12/2
* Gyro Angle input region 
* Diagonal movement
* implementing State machine
* Button behavior
* RF Module
* [Report](https://docs.google.com/document/d/1jMlAB727B3SJ2UlOs0SjzSUeTkuZW6ylENpsGi7Oyuk/edit)

# TODO 12/5
* Combinae Everything
* Everyone draw part of the State machine/ describe the behavior in LTL
* Record a demo video
* Frozen Hardware/Software
* Packaging
* Final Presentation Slide 
