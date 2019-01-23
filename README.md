# asp-servo-testapp
Test app for the asp-servo API

## Prerequisites

Install the asp-servo library

https://github.com/pjensfelt/asp_servo_api

## How to run

Clone the repository

```
git clone https://github.com/pjensfelt/asp_servo_testapp.git
cd asp-servo-testapp
mkdir build
cd build
cmake ..
make
```

There are three testapps that can be run

`sudo ./torquetest`

`sudo ./velocitytest`

`sudo ./positiontest`

The corresponding XML files in the config folder must be edited to contain the correct servo setup. The ethernet port can be detected by running:

`ifconfig`

The cycletime is defined in ms and can according to the HIWIN manual be set to 250, 500, 1000, 2000, or 4000 milliseconds. 

The testapp puts the ethercat master and the servos to operational state. Then the servos can be controlled by writing or reading entities to named servos, e.g.:

`servo_collection.write(servo_name,"Torque",target_torque);`

The entity names (in this case "Torque") are defined in the objects of the PDOmapping element in the XML config file. The servo name is defined in the name attribute of the servo XML object.




