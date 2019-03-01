#include "../include/asp_servo_testapp/velocitytest.hpp"

#define CTRL_LOOP 1000 
#define WRITE_MAX 100
#define WATCHDOG_LOOP 3000 // The watchdog has a loop that is twice as long as the main thread

int write_cnt = 0;
sem_t semaphore;

void *watchdog_fct(void *args){

	std::cout << "Watchdog function up" << std::endl;
	
	// Variables
	asp::ServoCollection *servo_collection = (asp::ServoCollection *)args;
	int prev_count = -2;
	int read_val = -1;
	
	// Wait the control thread to be operative
	sem_wait(&semaphore);
	
	do{		
		prev_count = read_val;
		read_val = write_cnt;
		std::this_thread::sleep_for(std::chrono::milliseconds(WATCHDOG_LOOP));
	}while(read_val != prev_count);
	
	// If we're here, the ctrl_fucntion hasn't updated the write count in a while -> emergency
	std::cout << "Watchdog TIMEOUT after " << WATCHDOG_LOOP << " ms,  stopping all" << std::endl;
	stop_all(servo_collection);
	
	std::cout << "Watchdog exiting "<< std::endl;
	pthread_exit(0);	
}

void stop_all(asp::ServoCollection *servo_collection){
	std::map<std::string, double>::iterator it;
	
	// Write 0 to all the servos
	for( it = scale.begin(); it!= scale.end(); it ++)
	{
		std::cout << "Stopping " << it->first << std::endl;
		servo_collection->write(it->first,"Velocity", 0);
	}
	// Or require that everyone goes into QUICK_STOP state
	//servo_collection->require_servo_state(asp::ServoStates::QuickStopActive);
}

void *ctrl_fct(void *args){
	
	std::cout << "Ctrl function up" << std::endl;
	
    // Prints out a summary of the servos
    asp::ServoCollection *servo_collection = (asp::ServoCollection *)args;
    std::cout << *servo_collection << std::endl;
 	bool verbose = false;
 	
    // Setting servo(s) in operational
    servo_collection->connect();
    servo_collection->require_ecat_state(asp::EthercatStates::Operational);
    servo_collection->require_servo_state(asp::ServoStates::OperationEnabled);

	// Signal the watchdog that I'm ready to send commands
	sem_post(&semaphore);
	
    // test loop
    int actual_velocity = 0;
    while (true) {
        std::cout << "Input (servo,targetvalue or q=quit): ";
        std::string input_str;
        std::cin >> input_str;
        if (input_str == "q") {
            break;
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
        
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(CTRL_LOOP));
        servo_collection->set_verbose(false);

	std::cout << "Position of " << servo_name << ": " 
		<< servo_collection->read_INT32(servo_name, "Position") 
		<< std::endl;
    }

    servo_collection->set_verbose(verbose);
    servo_collection->require_servo_state(asp::ServoStates::SwitchOnDisabled);
    servo_collection->set_verbose(false);  
    servo_collection->disconnect();
    std::cout << "Ctrl exiting" << std::endl;
    pthread_exit(0);
}

void cleanup(){	
    sem_destroy(&semaphore);
}

int main(int argc, char** argv) {
	
	pthread_t wdog_tid, ctrl_tid, main_tid;
	
    bool verbose = true;
    if (argc > 1) {
		verbose = false;
    }
    
	main_tid = pthread_self();
	
	// Loading servo settings from XML	- temporarly global
	asp::ServoCollection servo_collection(CONFIG + VEL_CONF);
	
	// Create semaphore
	sem_init(&semaphore, 0, 0);
	
    // Create watchdog thread
    if(pthread_create(&wdog_tid, NULL, watchdog_fct, (void *) &servo_collection) != 0){
    	std::cerr << "Error while creating watchdog, exiting" << std::endl;
    	cleanup();
    	exit(1);
    }
    
    // We have the watchdog -> create the control thread
    if(pthread_create(&ctrl_tid, NULL, ctrl_fct, (void *) &servo_collection) != 0){
    	std::cerr << "Error while creating control thread, exiting" << std::endl;
    	cleanup();
    	exit(1);
    }
    
    // Wait until the two threads have died
    if(pthread_join(wdog_tid, NULL) != 0){
    	std::cerr << "Error while joining watchdog thread, should never happen " << std::endl;
    	/* Error while waiting for watchdog thread, 
    	 *should never happen, perform safe exit just in case ?
    	 */
    	stop_all(&servo_collection);
    }
    
	/**
    * The watchdog has died (having performed the safe stop procedure), this 
    * *should* happen only if it detected that the control loop died/got stuck.
    * We can then kill the main thread (hence the control thread).
    */
    exit(0);
}
