Stationary Defense Turret

Description

This project features a stationary defense turret built using an Arduino microcontroller, servo motors, stepper motors, and ultrasonic sensors. The turret is designed to launch ping pong balls or training golf balls toward detected targets. It operates in both manual mode and autonomous mode, allowing either direct user control or automatic target detection and firing.

The system includes controlled rotation, adjustable launch angle, and a ball feeding mechanism capable of holding multiple projectiles. Autonomous functionality uses distance sensing to detect objects within range and respond with consistent aim and timed firing intervals.

Features

Manual control mode

Horizontal rotation range of ±90°
Adjustable launch angle between 20° and 80°
10-ball clip capacity
User-controlled aiming and firing

Autonomous targeting mode

Uses three ultrasonic sensors for environmental scanning
Detects targets within 3 meters
Automatically rotates toward detected objects
Fires at a fixed 25° launch angle
20-second delay between shots for controlled firing
20-ball clip capacity
Servo-controlled firing mechanism
Stepper motor-controlled rotation and elevation
Embedded C/C++ control logic

Hardware Used

Arduino microcontroller (Mega or Uno compatible)
Stepper motors (horizontal rotation and vertical angle control)
Servo motors (ball release and latch system)
Ultrasonic distance sensors (x3)
Motor drivers
battery pack
Ping pong balls

How It Works

The turret operates in two selectable modes:

Manual Mode:

The user directly controls turret positioning. Stepper motors allow the turret to rotate horizontally within a ±90° range, while vertical movement adjusts the firing angle between 20° and 80°. A servo-powered ball release mechanism fires projectiles from a 10-ball clip when triggered.

Autonomous Mode:

In autonomous operation, three ultrasonic sensors continuously measure distance to objects within the environment. When a target is detected within 3 meters, the turret automatically rotates toward the object and aligns itself at a fixed 25° firing angle optimized for projectile trajectory.

After firing, the system waits 20 seconds before launching the next ball. This delay ensures mechanical stability, reload time, and controlled operation. The autonomous system can hold up to 20 balls, allowing multiple engagements without manual reloading.

Stepper motors control movement accuracy, while servo motors provide fast and consistent ball release timing.

Repository Contents:

-Arduino source code

-Wiring diagrams

-Design documentation
