#pragma once
#include "csv.hpp"
#include "timespec.hpp"

// Defined in velocitytest.hpp
extern asp::ServoCollection servoCollection;
extern int writeCnt;
extern sem_t semaphore;	 			// Used to wake up the watchdog thread
extern pthread_mutex_t stopallMx; 	// Used to avoid more than one thread trying to stop the servos at the same time
extern bool stopped;				// At the beginning, the servos are stopped
extern const int CTRL_LOOP_MS;
extern const int MS_TO_NS;
const int WATCHDOG_LOOP_MS = 2*CTRL_LOOP_MS; // [ms] 

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

/**
* Attemp to provide a clean exit by executing stop_all when a signal is catched.
*/
void unregisterHandlers(){

	struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&(sa.sa_mask));
    for (int i = 1; i <= 64; i++) {
    	sigaddset(&(sa.sa_mask), i);
    	sigaction(i, &sa, NULL);
    }
}



/**
* Handles the received signals by stopping the servo motors.
*/
void handleSig(int sig){
	std::cerr << "Stopping. CAUGHT SIGNAL "<< strdup(strsignal(sig)) << std::endl;
	stopAll();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	unregisterHandlers();
	pthread_exit((void *) NULL);

}

/**
* Attemp to provide a clean exit by executing stop_all when a signal is catched.
*/
void registerHandlers(){

	struct sigaction sa;
    sa.sa_handler = handleSig;
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
void timespecDiff(struct timespec *start, struct timespec *stop,
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
        /*
		oldTime = newTime;
		clock_gettime(CLOCK_REALTIME, &newTime);
		timespecDiff(&oldTime, &newTime, &diff);
		
		std::cerr<<"Elapsed time: " << diff.tv_sec << " s " << diff.tv_nsec <<" ns " << std::endl;
		*/	   
		prevCount = readVal;
		readVal = writeCnt;
		std::cout << "HHH" << readVal << std::endl;
     
	}while(readVal != prevCount);
	
	// If we're here, the ctrl_fucntion hasn't updated the write count in a while -> emergency
	std::cout << "Stopping. Watchdog TIMEOUT after " << WATCHDOG_LOOP_MS << " ms,  stopping all" << std::endl;
	stopAll();
	
	std::cout << "Watchdog exiting "<< std::endl;
	pthread_exit(0);	
}
