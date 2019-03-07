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

const int CTRL_LOOP  	= 1000; // [ms]
const int WRITE_MAX  	= 100;  // [ms]
const int WATCHDOG_LOOP = 2000; // [ms] 

// Constants
const std::string CONFIG 	= "../config/" ;
const std::string VEL_CONF 	= "asp_test_velocity.xml" ;
const int INPUT_LEN 		= 255 ;

// Loading servo settings from XML	- temporarly global
asp::ServoCollection servo_collection(CONFIG + VEL_CONF);

int write_cnt 	= 0;
sem_t semaphore;	 		// Used to wake up the watchdog thread
pthread_mutex_t stopall_mx; // Used to avoid more than one thread trying to stop the servos at the same time
bool stopped = true;		// At the beginning, the servos are stopped


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

// --------------------------- Safety functions -----------------------------------//
/**
* Stops all the servo motors by requiring them to go in the QuickStopActive state.
* Turns them off and closes the connection.
*/
void stop_all(){
	
	pthread_mutex_lock(&stopall_mx);
	if(!stopped){

		std::cerr << "Stopping everything" << std::endl;
		// Or require that everyone goes into QUICK_STOP state
		servo_collection.require_servo_state(asp::ServoStates::QuickStopActive);

		// Shut everything down, disconnect
		servo_collection.require_servo_state(asp::ServoStates::SwitchOnDisabled);
		servo_collection.set_verbose(false);  
		servo_collection.disconnect();
		std::cerr << "Stopped everything" << std::endl;
		stopped = true;
	}
	pthread_mutex_unlock(&stopall_mx);
	return;
}

/**
* Handles the received signals by stopping the servo motors.
*/
void handle_sig(int sig){
	std::cerr << "CAUGHT SIGNAL "<< strdup(strsignal(sig)) << std::endl;
	stop_all();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	pthread_exit((void *) NULL);

}


/**
* Attemp to provide a clean exit by executing stop_all when a signal is catched.
*/
void register_handlers(){

	struct sigaction sa;
    sa.sa_handler = handle_sig;
    sigemptyset(&(sa.sa_mask));
    for (int i = 1; i <= 64; i++) {
    	sigaddset(&(sa.sa_mask), i);
    	sigaction(i, &sa, NULL);
    }
}

/**
* Computes the difference between 2 timespec structures.
* ref: https://gist.github.com/diabloneo/9619917
*/
void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

/**
* Implements the watchdog loop: periodically checks if a variable has been updated,
* which means that 
*/
void *watchdog_fct(void *args){

	register_handlers();
	std::cout << "Watchdog function up" << std::endl;
	
	// Variables
	asp::ServoCollection *servo_collection = (asp::ServoCollection *)args;
	int prev_count = -2;
	int read_val = -1;
	
	// Wait the control thread to be operative
	sem_wait(&semaphore);
	struct timespec old_time, new_time, diff;
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    
    // Initiali time
    clock_gettime(CLOCK_REALTIME, &new_time);
		
	do{		
		old_time = new_time;
		clock_gettime(CLOCK_REALTIME, &new_time);
		timespec_diff(&old_time, &new_time, &diff);
		
		std::cerr<<"Elapsed time: " << diff.tv_sec << " s " << diff.tv_nsec <<" ns " << std::endl;
			   
		prev_count = read_val;
		read_val = write_cnt;
     
		std::this_thread::sleep_for(std::chrono::milliseconds(WATCHDOG_LOOP));
	}while(read_val != prev_count);
	
	// If we're here, the ctrl_fucntion hasn't updated the write count in a while -> emergency
	std::cout << "Watchdog TIMEOUT after " << WATCHDOG_LOOP << " ms,  stopping all" << std::endl;
	stop_all();
	
	std::cout << "Watchdog exiting "<< std::endl;
	pthread_exit(0);	
}



