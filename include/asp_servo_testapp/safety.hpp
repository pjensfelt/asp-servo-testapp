#pragma once
#include "csv.hpp"
#include "timespec.hpp"
#include "ServoInfo.hpp"
#include <cmath>

// Defined in csv.hpp
extern asp::ServoCollection servoCollection;
extern std::map<std::string, cmd::ServoInfo> servoInfo_;
extern int writeCnt;
extern sem_t semaphore;	 			// Used to wake up the watchdog thread
extern pthread_mutex_t stopallMx; 	// Used to avoid more than one thread trying to stop the servos at the same time
extern bool stopped;				// At the beginning, the servos are stopped
extern const int CTRL_LOOP_MS;
extern const int MS_TO_NS;
const int WATCHDOG_LOOP_MS = 2*CTRL_LOOP_MS; // [ms] 

// Workspace limits
const double X_LIM[2]	= {.0, 2.750}	; // [m]
const double Y_LIM[2]	= {.0, 100.0}	; // [m] y has no upper bound
const double X_OFFSET	= 0.165		; // Translation from global origin to 
const double Y_OFFSET	= 0.1025		; // arm origin

const double P1[2]		= {0.0625+0.1, 0.962}; // Half arm width + gearbox width; arm lenght
const double P2[2]		= {-0.0625   , 0.962}; // Symmetric, no gearbox

/**
* Stops all the servo motors by requiring them to go in the QuickStopActive state.
* Turns them off and closes the connection.
*/
void stopAll(){
	
	pthread_mutex_lock(&stopallMx);
	if(!stopped){

		std::cerr << "Stopping everything" << std::endl;
		// Require that everyone goes into QUICK_STOP state
		servoCollection.require_servo_state(asp::ServoStates::QuickStopActive);

		// Shut everything down, disconnect
		servoCollection.require_servo_state(asp::ServoStates::SwitchOnDisabled);
		servoCollection.set_verbose(false);  
		servoCollection.disconnect();
		std::cerr << "Stopped everything" << std::endl;
		stopped = true;
	}
	pthread_mutex_unlock(&stopallMx);
	return;
}

void checkInWorkspace()
{
	int pos;
	double x, y, theta;
	double P1_p[2], P2_p[2];
	x = servoInfo_["s2"].SIfromTicks(servoCollection.read_INT32("s2", "Position"));
	y = servoInfo_["s3"].SIfromTicks(servoCollection.read_INT32("s3", "Position"));
	theta = servoInfo_["s4"].SIfromTicks(servoCollection.read_INT32("s4", "Position"));
	
	P1_p[0] = P1[0]*cos(theta) - P1[1]*sin(theta) + X_OFFSET + x;
	P1_p[1] = P1[0]*sin(theta) + P1[1]*cos(theta) + Y_OFFSET + y;
	P2_p[0] = P2[0]*cos(theta) - P2[1]*sin(theta) + X_OFFSET + x;
	P2_p[1] = P2[0]*cos(theta) - P2[1]*sin(theta) + Y_OFFSET + y;
	
	std::cout << "P1x " << P1_p[0] << "P1y " << P1_p[1] << std::endl;
	std::cout << "P2x " << P2_p[0] << "P2y " << P2_p[2] << std::endl;
	
	if(    P1_p[0]< X_LIM[0] || P1_p[0]> X_LIM[1]  /*P1.x is outbounds*/
		|| P2_p[0]< X_LIM[0] || P2_p[0]> X_LIM[1]  /*P2.x is outbounds*/
		|| P1_p[1]< Y_LIM[0] || P1_p[1]> Y_LIM[1]  /*P1.y is outbounds*/
		|| P2_p[1]< Y_LIM[0] || P2_p[1]> Y_LIM[1]){/*P2.y is outbounds*/
			stopAll();
		}
	
	for (auto kvp: servoInfo_) {
        pos = servoCollection.read_INT32(kvp.first, "Position");
        kvp.second.setPositionTicks(pos); // For logging when exiting
        if(!kvp.second.inLimitsTicks(pos)){
            std::cout << "Stopping. Servo " << kvp.first << " not in limits" << std::endl;
            std::cout << "Position " << pos << " limits " << kvp.second.getLlimTicks() << " " << kvp.second.getUlimTicks();
            stopAll();
            break;
        }
    }
}


/**
* Implements the watchdog loop: periodically checks if a variable has been updated,
* which means that 
*/
void *watchdogFct(void *args){

	//registerHandlers();
	std::cout << "Watchdog function up" << std::endl;
	
	// Variables
	asp::ServoCollection *servo_collection = (asp::ServoCollection *)args;
	int prevCount = -2;
	int readVal = -1;
	
	struct timespec oldTime, newTime, diff;
	struct timespec tspec, remaining;
    int64_t cycletime_ns = WATCHDOG_LOOP_MS * MS_TO_NS;	// From ms to ns
    long            ms; // Milliseconds
    time_t          s;  // Seconds
	
	// Wait the control thread to be operative
	sem_wait(&semaphore);
	std::cout << "Now watchdog awake " << std::endl;
    
    // Initiali time
    //clock_gettime(CLOCK_REALTIME, &newTime);
    clock_gettime(CLOCK_MONOTONIC, &tspec);	
	do{		
		// Adding the cycletime to the timespec
        timespec_add(&tspec, cycletime_ns);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, &remaining);
        	   
		prevCount = readVal;
		readVal = writeCnt;
     
	}while(readVal != prevCount);
	
	// If we're here, the ctrl_fucntion hasn't updated the write count in a while -> emergency
	std::cout << "Stopping. Watchdog TIMEOUT after " << WATCHDOG_LOOP_MS << " ms,  stopping all" << std::endl;
	stopAll();
	
	std::cout << "Watchdog exiting "<< std::endl;
	pthread_exit(0);	
}
