#include "../include/asp_servo_testapp/csv.hpp"

// Will be const pointer
void cmdCallback(/*std::array<VelocityCommand, N_SERVO> &cmds*/)
{       
    int velCmd, maxVelCmd;
    
    /**
    * Create fake commands just to try
    */
    std::array<VelocityCommand, N_SERVO> cmds;
    cmds[0] = VelocityCommand("s1", +0.01, 0.00); 	// Z 1 cm/s, 1 cm/s
    cmds[1] = VelocityCommand("s2", 0.0, 0.00); 	// X 0  cm, 0 cm/s X
    cmds[2] = VelocityCommand("s3", 0.0, 0.00); 	// Y 0  cm, 0 cm/s
    cmds[3] = VelocityCommand("s4", 0.0, 0.00); 	// B 0  cm, 0 cm/s
    cmds[4] = VelocityCommand("s5", 0.0, 0.00); 	// A 0  cm, 0 cm/s

	for(const auto cmd: cmds){
		servoCollection.sendVelCmdSI(cmd.servoName, cmd.targetVelocity);
	}
	

    // Say to the watchdog that we've  sent the commands
    writeCnt =(writeCnt +1) % WRITE_MAX;        
}


void *ctrlFct(void *args){
	
	// Variables
    struct timespec tspec, remaining;
    int64_t cycletime_ns = CTRL_LOOP_MS * MS_TO_NS;	// From ms to ns


    clock_gettime(CLOCK_MONOTONIC, &tspec);

	//registerHandlers();
	std::cout << "Ctrl function up" << std::endl;
	
    // Prints out a summary of the servos
    std::cout << servoCollection << std::endl;
 	bool verbose = false;
 	
    // Setting servo(s) in operational
    servoCollection.switchOnServo();
	
    int i = 0;

    // test loop
    while (true) {
		// Signal the watchdog that I'm ready to send commands // HERE OR LOWER?
		sem_post(&semaphore); // Only needed once, but branches cost and are ugly. ( Do they tho ?)

        // Adding the cycletime to the timespec
        timespec_add(&tspec, cycletime_ns);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, &remaining);
        
        // Call "fake callback" function, will be subscribed to topic
		cmdCallback();
        i++;
    }
 
    std::cout << "Ctrl exiting" << std::endl;
    pthread_exit(0);
}

void cleanup()
{	
    sem_destroy(&semaphore);
}

int main(int argc, char** argv) 
{
	pthread_t wdogTid, ctrlTid;

    //registerHandlers();

    bool verbose = true;
    if (argc > 1) {
		verbose = false;
    }
    
	// Initialize synchronization elements
	sem_init(&semaphore, 0, 0);
	
    // Create watchdog thread
    if(pthread_create(&wdogTid, NULL, watchdogFct, (void *) &servoCollection) != 0){
    	std::cerr << "Error while creating watchdog, exiting" << std::endl;
    	cleanup();
    	exit(1);
    }
    // Set thread name
    pthread_setname_np(wdogTid, "Watchdog");
    
    // We have the watchdog -> create the control thread
    if(pthread_create(&ctrlTid, NULL, ctrlFct, (void *) &servoCollection) != 0){
    	std::cerr << "Error while creating control thread, exiting" << std::endl;
    	cleanup();
    	exit(1);
    }
    
    // Set thread name
    pthread_setname_np(ctrlTid, "Control");
    
    // Wait until the two threads have died
    if(pthread_join(wdogTid, NULL) != 0){
    	std::cerr << "Stopping. Error while joining watchdog thread, should never happen " << std::endl;
    	/* Error while waiting for watchdog thread, 
    	 *should never happen, perform safe exit just in case ?
    	 */
    	servoCollection.stopAll();
    }
    std::cerr << "Watchdog died" << std::endl;
	/**
    * The watchdog has died (having performed the safe stop procedure), this 
    * *should* happen only if it detected that the control loop died/got stuck.
    * We can then kill the main thread (hence the control thread).
    */
	cleanup();
    exit(0);
}
