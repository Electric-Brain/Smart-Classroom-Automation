Smart Classroom Automation System

A fully local, distributed Smart Classroom Automation and Monitoring System built using a triple-ESP32 embedded architecture.

This system modernizes traditional classrooms by automating shutdown procedures, improving security, reducing energy waste, and simplifying operational effort for school support staff.

🎯 Problem Statement

In many schools, after class hours, the responsibility of shutting down classroom infrastructure falls on the support staff (peon).

They must manually:

Turn off lights

Switch off fans

Close windows

Close curtains

Ensure doors are secured

This repetitive manual process is time-consuming and prone to human error.
If even one device is left ON or open:

Energy is wasted

Security is compromised

Operational efficiency decreases

Affordable schools often lack centralized automation systems due to high commercial costs.

💡 Our Solution

We developed a cost-effective Smart Classroom Automation System that provides:

One-touch master shutdown

Centralized web-based dashboard

Manual physical override button

Automated ventilation and door control

Live CCTV monitoring

Real-time environmental tracking

The system significantly reduces manual effort and ensures consistent shutdown procedures.

🚀 Core Features
🎛 Smart Display with Audio

3.5" ILI9488 TFT Smart Board Display

Real-time system status visualization

DFPlayer Mini–based audio announcements

Context-based system alerts

🪟 Smart Window System

Dual-mode automation:

Sliding window control

Outward opening window control

Temperature-based ventilation logic

Dashboard + manual control

🪟 Smart Curtain Automation

Motorized curtain system

Web-controlled open/close

Integrated with master shutdown logic

🚪 Smart Door System

Servo/motor-driven mechanism

IR/PIR-based automatic opening

Manual dashboard control

Safety-aware logic

💡 Smart Lighting Control

Relay-based light automation

Remote ON/OFF control

Included in master shutdown

🌬 Smart Fan Control

Relay-based control

Integrated with dashboard and master OFF system

🎥 Smart Camera & CCTV

ESP32-CAM surveillance module

Live MJPEG streaming over local network

Flash LED control

Embedded live feed inside dashboard

🌡 Environmental Monitoring

DHT22 temperature & humidity sensor

Real-time dashboard updates

Displayed on TFT screen

🖐 Master Touch Control

Hardware touch interface (TTP223)

One-touch control for:

Lights

Fan

Windows

Curtains

Emergency full shutdown function

🌐 Smart Web Dashboard (Local Server)

Fully local operation (No Cloud)

Individual device control

Real-time sensor monitoring

Embedded CCTV live feed

Low-latency internal network system

🧠 System Architecture

This project uses a distributed 3-ESP32 architecture:

ESP32 #1 – Main Controller

Hosts Local Web Server

Processes Sensors (DHT22, PIR, Touch)

Controls Relays (Lights & Fan)

Acts as UART Communication Hub

Sends commands to Motor and Display controllers

ESP32 #2 – Motor Controller

Controls TB6612 Motor Drivers

Drives DC Motors (Curtains & Windows)

Controls Servos (Door & Window)

Reads Reed Switch feedback

Implements safety timeouts

ESP32 #3 – Display & Audio Controller

Drives ILI9488 TFT

Controls DFPlayer Mini

Displays real-time system status

Provides UI feedback

ESP32-CAM – Surveillance Module

Independent camera controller

Live streaming over WiFi

Integrated into dashboard

Communication between controllers is handled using a UART protocol.

🛠 Hardware Stack

3 × ESP32 Development Boards

1 × ESP32-CAM (AI Thinker)

ILI9488 3.5" SPI TFT Display

DFPlayer Mini + Speaker

TB6612 Motor Drivers

N20 Gear Motors

Servo Motors

DHT22 Temperature Sensor

PIR / IR Motion Sensor

TTP223 Touch Sensor

Relay Modules

Local 5V Regulated Power System

🧩 Software Stack

Arduino Framework (ESP32 Core)

WiFi + Local Web Server

UART Inter-Controller Protocol

TFT_eSPI Library

DFPlayer Serial Library

Non-blocking firmware design

State-based control architecture

🏛 Project Showcase & Recognition

This project was developed under an innovation initiative supported by Agastya International Foundation, in collaboration with industry mentors from JP Morgan.

The system was showcased at:

📍 Yashwantrao Chavan Center

Presented before:

Industry mentors (JP Morgan)

Scientific evaluators

Innovation panel members

The project received positive feedback for:

Practical real-world problem solving

Modular embedded system design

Cost-effective implementation

👨‍🏫 Collaborative Development

This system was developed in collaboration with a 9th standard student as part of a hands-on STEM innovation initiative.

The project aimed to:

Introduce embedded systems at school level

Encourage practical engineering thinking

Demonstrate real-world problem solving

Promote early-stage innovation culture

This reflects a mentorship-driven development model combining advanced system architecture with grassroots education impact.

🎯 Target Applications

Private Schools

Coaching Institutes

Budget Smart Classrooms

Government Digital School Initiatives

🔮 Future Improvements

Custom PCB integration

Multi-classroom centralized monitoring

Energy analytics system

Cloud optional upgrade

AI-based occupancy optimization

📄 License

Released under the MIT License.