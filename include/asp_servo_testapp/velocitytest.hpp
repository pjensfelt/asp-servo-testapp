#include <asp_servo_api/asp_servo.h>
#include <asp_servo_api/servo.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <csignal>
#include <string.h>
#include <semaphore.h>

#define MAX_V 		1310720

// Constants
const std::string CONFIG 	= "../config/" ;
const std::string VEL_CONF 	= "asp_test_velocity.xml" ;
const int INPUT_LEN 		= 255 ;

// Maps non-standard name of a servo, to the standard convention
std::map<std::string, std::string> servoname = 
	{
		{"s1", "s1"},
		{"s2", "s2"},
		{"s3", "s3"},
		{"s4", "s4"},
		{"s5", "s5"},

		{"S1", "s1"},
		{"S2", "s2"},
		{"S3", "s3"},
		{"S4", "s4"},
		{"S5", "s5"},

		{"z", "s1"},
		{"x", "s2"},
		{"y", "s3"},
		{"b", "s4"},
		{"a", "s5"},

		{"Z", "s1"},
		{"X", "s2"},
		{"Y", "s3"},
		{"B", "s4"},
		{"A", "s5"}	
	};

std::map<std::string, double> scale = 
	{

		{"s1",  5044.0}, // Z -> ticks/mm 
		{"s2",  3991.8}, // X -> ticks/mm
		{"s3",  5099.4}, // Y -> ticks/mm
		{"s4", 32768.0}, // B -> ticks/°
		{"s5", 27307.0}, // A -> ticks/°
	};

/**
* Parses a command and fills the value of servo and velocity.
* The accepted format is:
* servoname velocity
* Velocity is expressed in mm/s or °/s
* Servoname can either be s1/s2/../S1/S2../x/y/.../X/Y/... as in the servoname map.
*/
void parse_command (std::string command, std::string &servo, float &vel)
{
	std::string delimiter = " ";
	size_t pos;
	std::map<std::string, std::string>::iterator it;
	
	pos = command.find(delimiter);
	
	// There is no " " in the string, or it is at the beginning of the command
	if(pos == std::string::npos || pos == 0)
	{
		std::cout << "Command:" << command << std::endl;
		throw std::runtime_error("Cannot parse the command \n");
	}
	
	std::string name = command.substr(0, pos); 
	it = servoname.find(name);
	
	// Suitable conversion for the servo name not found
	if(it == servoname.end())
	{
		throw std::runtime_error("Unknown servo name \n");
	}
	servo = it->second;
	
	try
	{
		vel = std::stof(command.substr(pos+1, command.size() - pos - 1)); 
	} catch (std::invalid_argument e){
		throw std::runtime_error("Cannot parse command. Check velocity format.\n");
	}

}

/**
* Converts a velocity into the number of ticks/s to be inputted to the servo motor.
* The velocity has to be expressed in mm/s in case of s1, s2, s3 or °/s in case of A and B.
*/
int to_ticks(float velocity, std::string servo)
{
	std::map<std::string, double>::iterator it = scale.find(servo);
	if(it == scale.end())
	{
		throw std::runtime_error("This error is due to a programming error, check it out!\n");
	}
	std::cout << "Scale factor " << it->second << std::endl;
	return (int) it->second*velocity;
}

void stop_all();

void stop_all(int sig);

/**
* Attemp to provide a clean exit by executing stop_all when a signal is catched.
*/
void register_handlers(){

	struct sigaction sa;
    sa.sa_handler = stop_all;
    sigemptyset(&(sa.sa_mask));
    for (int i = 1; i <= 64; i++) {
    	sigaddset(&(sa.sa_mask), i);
    	sigaction(i, &sa, NULL);
    }
}
