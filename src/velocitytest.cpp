#include "../include/asp_servo_testapp/velocitytest.hpp"

void *ctrl_fct(void *args){
	
	register_handlers();
	std::cout << "Ctrl function up" << std::endl;
	
    // Prints out a summary of the servos
    asp::ServoCollection *servo_collection = (asp::ServoCollection *)args;
    std::cout << *servo_collection << std::endl;
 	bool verbose = false;
 	
    // Setting servo(s) in operational
    servo_collection->connect();
    servo_collection->require_ecat_state(asp::EthercatStates::Operational);
    servo_collection->require_servo_state(asp::ServoStates::OperationEnabled);
    
	// Update the servo state to not-stopped
	pthread_mutex_lock(&stopall_mx);
	stopped = false;
	pthread_mutex_unlock(&stopall_mx);
	
	// Signal the watchdog that I'm ready to send commands // HERE OR LOWER?
	sem_post(&semaphore);
	
    // test loop
    int actual_velocity = 0;
    while (true) {
    	
    	std::cerr << "Input (servo,targetvalue or q=quit): ";
        std::string input_str;
        std::cin >> input_str;
        if (input_str == "q") {
			stop_all();
            break;
        }else if(input_str == "s"){
        	// Raise signal
        	raise(SIGABRT);
        }
        std::string delimiter = ",";
        std::string servo_name = input_str.substr(0, input_str.find(delimiter));

	std::cout << "Position of " << servo_name << ": " 
		<< servo_collection->read_INT32(servo_name, "Position") 
		<< std::endl;

        int target_velocity = std::stoi(input_str.substr(input_str.find(delimiter)+1,input_str.size() - input_str.find(delimiter) - 1));
        if (target_velocity > 1310720) {
            target_velocity = 1310720;
        }
        if (target_velocity < -1310720) {
            target_velocity = -1310720;
        }

        servo_collection->set_verbose(verbose);
        // Writing velocity to servo
        servo_collection->write(servo_name,"Velocity",target_velocity);
        
        // Say to the watchdog that we've sent the command
        write_cnt =(write_cnt +1) % WRITE_MAX;
        
        //raise(SIGABRT);
        
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(CTRL_LOOP));
        
        servo_collection->set_verbose(false);

	std::cout << "Position of " << servo_name << ": " 
		<< servo_collection->read_INT32(servo_name, "Position") 
		<< std::endl;
		
    }

    std::cout << "Ctrl exiting" << std::endl;
    pthread_exit(0);
}

void cleanup(){	
    sem_destroy(&semaphore);
}

int main(int argc, char** argv) {
	
	//register_handlers();
	pthread_t wdog_tid, ctrl_tid, main_tid;
	
    bool verbose = true;
    if (argc > 1) {
		verbose = false;
    }
    
	main_tid = pthread_self();
	
	// Initialize synchronization elements
	sem_init(&semaphore, 0, 0);
	pthread_mutex_init(&stopall_mx, NULL);
	
    // Create watchdog thread
    if(pthread_create(&wdog_tid, NULL, watchdog_fct, (void *) &servo_collection) != 0){
    	std::cerr << "Error while creating watchdog, exiting" << std::endl;
    	cleanup();
    	exit(1);
    }
    // Set thread name
    pthread_setname_np(wdog_tid, "Watchdog");
    
    // We have the watchdog -> create the control thread
    if(pthread_create(&ctrl_tid, NULL, ctrl_fct, (void *) &servo_collection) != 0){
    	std::cerr << "Error while creating control thread, exiting" << std::endl;
    	cleanup();
    	exit(1);
    }
    
    // Set thread name
    pthread_setname_np(ctrl_tid, "Control");
    
    // Wait until the two threads have died
    if(pthread_join(wdog_tid, NULL) != 0){
    	std::cerr << "Error while joining watchdog thread, should never happen " << std::endl;
    	/* Error while waiting for watchdog thread, 
    	 *should never happen, perform safe exit just in case ?
    	 */
    	stop_all();
    }
	/**
    * The watchdog has died (having performed the safe stop procedure), this 
    * *should* happen only if it detected that the control loop died/got stuck.
    * We can then kill the main thread (hence the control thread).
    */
	cleanup();
    exit(0);
}
