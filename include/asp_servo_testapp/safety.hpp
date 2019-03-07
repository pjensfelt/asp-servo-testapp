#ifndef SAFETY_H
#define SAFETY_H
#include "velocitytest.hpp"


// Defined in velocitytest.hpp
extern asp::ServoCollection servo_collection;
extern const int WATCHDOG_LOOP; // [ms] 
extern int write_cnt;
extern sem_t semaphore;	 			// Used to wake up the watchdog thread
extern pthread_mutex_t stopall_mx; 	// Used to avoid more than one thread trying to stop the servos at the same time
extern bool stopped;				// At the beginning, the servos are stopped

/**
* Stops all the servo motors by requiring them to go in the QuickStopActive state.
* Turns them off and closes the connection.
*/
void stop_all(){
	
	pthread_mutex_lock(&stopall_mx);
	if(!stopped){

		std::cerr << "Stopping everything" << std::endl;
		// Require that everyone goes into QUICK_STOP state
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
#endif

