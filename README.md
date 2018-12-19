# asp-servo-testapp
Test app for the asp-servo API

## Prerequisites

Install the asp-servo library

https://github.com/urban-eriksson/asp-servo

## How to run

Clone the repository

```
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

