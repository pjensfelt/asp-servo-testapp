#include "../include/asp_servo_testapp/csv.hpp"

// Will be const pointer
void cmdCallback(/*std::array<VelocityCommand, N_SERVO> &cmds*/)
{       
    int velCmd, maxVelCmd;
    std::map<std::string, cmd::ServoInfo>::iterator sIt;
    
    /**
    * Create fake commands just to try
    */
    std::array<VelocityCommand, N_SERVO> cmds;
    cmds[0] = VelocityCommand("s1", 0.01, 0.01); // 1 cm/s, 1 cm/s
    cmds[1] = VelocityCommand("s2", 0.0, 0.00); // 0  cm, 0 cm/s
    cmds[2] = VelocityCommand("s3", 0.0, 0.00); // 0  cm, 0 cm/s
    cmds[3] = VelocityCommand("s4", 0.0, 0.00); // 0  cm, 0 cm/s
    cmds[4] = VelocityCommand("s5", 0.0, 0.00); // 0  cm, 0 cm/s
    
    for(const auto cmd: cmds){
        // Get the ServoInfo of interest
        sIt = servoInfo_.find(cmd.servoName);
        if(sIt == servoInfo_.end()){
            std::cerr << "Programming error, unknown servo name " << cmd.servoName << std::endl;
            break; 
        }
        velCmd = sIt->second.toTicks(cmd.targetVelocity);
        maxVelCmd = sIt->second.toTicks(cmd.maxProfVelocity);
        servoCollection.write(cmd.servoName,"Velocity",velCmd);
            
        //servoCollection.write(cmd.servoName,"Position", posCmd);
        //servoCollection.get_servo(0)->SDOwrite_INT32(sIt->second.getNumber(), 0x6081, 0, velCmd);
        
    }

    // Say to the watchdog that we've sent the commands
    writeCnt =(writeCnt +1) % WRITE_MAX;
    
    // Check servo position
    checkServoPosition();
        
}


void *ctrlFct(void *args){
	
	// Variables
	int 			actualVelocity 	= 0;
    float 			targetVelocity 	= 0; // in [mm/s] or [°/s]
    int 			servoCmd 		= 0;
    struct timespec tspec, remaining;
    int64_t cycletime_ns = CTRL_LOOP_MS * MS_TO_NS;	// From ms to ns


    clock_gettime(CLOCK_MONOTONIC, &tspec);

	//registerHandlers();
	std::cout << "Ctrl function up" << std::endl;
	
    // Prints out a summary of the servos
    std::cout << servoCollection << std::endl;
 	bool verbose = false;
 	
    // Setting servo(s) in operational
    switchOnServo();
	
    int i = 0;

    // test loop
    while (true) {
    	
		// Signal the watchdog that I'm ready to send commands // HERE OR LOWER?
		sem_post(&semaphore); // Only needed once, but branches cost and are ugly. ( Do they tho ?)
		/*if(i == 1000){ // CHECK when delay is detected
            std::cout << "Sleeping more" << std::endl;
            timespec_add(&tspec, cycletime_ns*3);
        }*/
        // Adding the cycletime to the timespec
        timespec_add(&tspec, cycletime_ns);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tspec, &remaining);
        
        // Call "fake callback" function, will be subscribed to topic
		cmdCallback();
        i++;
        if(i== 10){
        	break;
        }
    }
 
    std::cout << "Ctrl exiting" << std::endl;
    pthread_exit(0);
}

void checkServoPosition()
{
    int pos = 0;
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

void switchOnServo()
{
    servoCollection.connect();
    servoCollection.require_ecat_state(asp::EthercatStates::Operational);
    servoCollection.require_servo_state(asp::ServoStates::OperationEnabled);
    
	// Update the servo state to not-stopped
	pthread_mutex_lock(&stopallMx);
	stopped = false;
	pthread_mutex_unlock(&stopallMx);
}

void initServoInfo(){
    // XML config file...
    /*servoInfo_["s1"] = cmd::ServoInfo("s1", 1, "Z", 5044000.00, "m/s",   (int)(0.052*5044000.0),  (int)(1.3*5044000.0)); 
    servoInfo_["s2"] = cmd::ServoInfo("s2", 2, "X", 3991800.00, "m/s",   (int)(0.122*3991800.0),  (int)(1.5*3991800.0)); 
    servoInfo_["s3"] = cmd::ServoInfo("s3", 3, "Y", 5099400.00, "m/s",   (int)(0.080*5099400.0),  (int)(0.5*5099400.0)); 
    servoInfo_["s4"] = cmd::ServoInfo("s4", 4, "B", 1877468.10, "rad/s", (int)(0.0  *1877468.0),  (int)(M_PI*1877468.0)); 
    servoInfo_["s5"] = cmd::ServoInfo("s5", 5, "A", 1564575.85, "rad/s", (int)(0.0 *1564575.85),  (int)(M_PI*1564575.85)); 
    */

    servoInfo_["s1"] = cmd::ServoInfo("s1", 1, "Z", 5044000.00, "m/s",   (int)(0.2*5044000.00),  (int)(1.3*5044000.0)); 
    servoInfo_["s2"] = cmd::ServoInfo("s2", 2, "X", 3991800.00, "m/s",   (int)(0.3*3991800.00),  (int)(1.5*3991800.0)); 
    servoInfo_["s3"] = cmd::ServoInfo("s3", 3, "Y", 5099400.00, "m/s",   (int)(0*5099400.00),  (int)(0.5*5099400.0)); 
    servoInfo_["s4"] = cmd::ServoInfo("s4", 4, "B", 1877468.10, "rad/s", (int)(-(0.8)*M_PI/2*1877468.0),  (int)(+(0.8)*M_PI/2*1877468.0)); 
    servoInfo_["s5"] = cmd::ServoInfo("s5", 5, "A", 1564575.85, "rad/s", (int)0,  (int)(M_PI*1564575.85)); 
}

void cleanup()
{	
    sem_destroy(&semaphore);
    pthread_mutex_destroy(&stopallMx);
}

int main(int argc, char** argv) 
{
	pthread_t wdogTid, ctrlTid;

    //registerHandlers();

    bool verbose = true;
    if (argc > 1) {
		verbose = false;
    }
    // Initialize the array of servo info
    initServoInfo();
    
	// Initialize synchronization elements
	sem_init(&semaphore, 0, 0);
	pthread_mutex_init(&stopallMx, NULL);
	
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
    	stopAll();
    }
    std::cerr << "Watchdog died" << std::endl;
	/**
    * The watchdog has died (having performed the safe stop procedure), this 
    * *should* happen only if it detected that the control loop died/got stuck.
    * We can then kill the main thread (hence the control thread).
    */

    // Print final positions

    std::cerr << "Final positions" << std::endl;
    for (auto kvp: servoInfo_) {    
        std::cerr << "Servo " << kvp.first << " : " << kvp.second.getPositionSI() << std::endl;
    }
	cleanup();
    exit(0);
}
