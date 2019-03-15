#pragma once
#include <asp_servo_api/asp_servo.h>
#include <asp_servo_api/servo.h>
#include <tinyxml2.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <map>

#include <csignal>
#include <array>
#include <semaphore.h>
#include "safety.hpp"
#include "ServoInfo.hpp"
#include "timespec.hpp"

const int CTRL_LOOP_MS  	= 10; 						// [ms]
const int WRITE_MAX  		= 100;  					// [ms]
const int N_SERVO 			= 5;						// Number of servo motors
const int MS_TO_NS			= 1000000; 					// To convert ms to ns

// Constants
const std::string CONFIG 		= "../config/" ;
const std::string CSV_CONF 		= "asp_test_position.xml" ;

// Loading servo settings from XML
asp::ServoCollection servoCollection(CONFIG + CSV_CONF);
std::map<std::string, cmd::ServoInfo> servoInfo_;

int writeCnt 	= 0;
sem_t semaphore;	 		// Used to wake up the watchdog thread
pthread_mutex_t stopallMx; 	// Used to avoid more than one thread trying to stop the servos at the same time
bool stopped = true;		// At the beginning, the servos are stopped


/**
* Represents a command that has to be given to the servo motors. 
* Command are generated as an array, with one command for each servo motor.
*/
struct VelocityCommand
{
	std::string servoName;
	double targetVelocity;
	double maxProfVelocity;
	VelocityCommand(){}
	VelocityCommand(std::string _servoName, double _targetVelocity, double _maxProfVelocity){
		servoName 		= _servoName;
		targetVelocity 	= _targetVelocity;
		maxProfVelocity = _maxProfVelocity;
	}
};

/**
* Establishes a connection with the servo motors master and switches them on.
*/
void switchOnServo();

/**
* Callback called when a new command is published.
*/
void cmdCallback();

/**
* Checks if the servo motors are in an admissible position. 
*/
void checkServoPosition();
