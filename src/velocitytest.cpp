#include "../include/asp_servo_testapp/velocitytest.hpp"


int main(int argc, char** argv) {

	// Register signal handlers to be executed when catching SIGINT/SIGABORT/...
	// Attempt to implement a safe exit
	register_handlers();
	
    bool verbose = true;
    if (argc > 1) {
		verbose = false;
    }
    
	// Variables
	int 	actual_velocity 	= 0;
    float 	target_velocity 	= 0; // in [mm/s] or [°/s]
    int 	servo_cmd 			= 0;
    char 	input_str[INPUT_LEN];
    std::string servo_name;	
	

    // Prints out a summary of the servos
    std::cout << servo_collection << std::endl;
 
    // Setting servo(s) in operational
    servo_collection.connect();
    servo_collection.require_ecat_state(asp::EthercatStates::Operational);
    servo_collection.require_servo_state(asp::ServoStates::OperationEnabled);
	
    // test loop    
    while (true) {
        std::cout << "Input (servo targetvalue or q=quit): ";
        std::cin.getline(input_str, INPUT_LEN);
        
        if (strcmp(input_str, "q")==0) {
            break;
        }
        
        try {
        	parse_command(input_str, servo_name, target_velocity);
        
			std::cout << "Position of " << servo_name << ": " 
				<< servo_collection.read_INT32(servo_name, "Position") 
				<< std::endl;

		    // Transform from [mm/s]/[°/s] to a proper command for the servo motor
		    servo_cmd = to_ticks(target_velocity, servo_name);
		} catch(const std::runtime_error& error){
			std::cerr<< error.what();
			// Should we stop the servo? Which one? Everyone?
			stop_all();
			continue;
		}

		// chop the command. Is this MAX_V the right one for every servo?
        if (servo_cmd > MAX_V) {
            servo_cmd = MAX_V;
        }
        if (servo_cmd < -MAX_V) {
            servo_cmd = -MAX_V;
        }
        
        std::cout << "Writing to servo " << servo_name << " ticks: "<< servo_cmd << std::endl;
		
        servo_collection.set_verbose(verbose);
        
        // Writing velocity to servo
        servo_collection.write(servo_name,"Velocity",servo_cmd);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        servo_collection.set_verbose(false);

		std::cout << "Position of " << servo_name << ": " 
			<< servo_collection.read_INT32(servo_name, "Position") 
			<< std::endl;
			
		// Raise signals just to see if it exits cleanly
		//std::raise(SIGABRT);
    }
	
    servo_collection.set_verbose(verbose);
    servo_collection.require_servo_state(asp::ServoStates::SwitchOnDisabled);
    servo_collection.set_verbose(false);  
    servo_collection.disconnect();
	
    return 0;
}


