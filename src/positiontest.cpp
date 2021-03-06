#include <asp_servo_api/asp_servo.h>
#include <asp_servo_api/servo.h>
#include <iostream>
#include <chrono>
#include <thread>

int main() {

    // Loading servo settings from XML
    asp::ServoCollection servo_collection("../config/asp_test_position.xml");

    // Prints out a summary of the servos
    std::cout << servo_collection << std::endl;

    // Setting servo(s) in operational
    servo_collection.connect();
    servo_collection.require_ecat_state(asp::EthercatStates::Operational);
    servo_collection.require_servo_state(asp::ServoStates::OperationEnabled);

    // test loop
    while (true) {
        std::cout << "Input (servo,targetvalue or q=quit): ";
        std::string input_str;
        std::cin >> input_str;
        if (input_str == "q") {
            break;
        }
        std::string delimiter = ",";
        std::string servo_name = input_str.substr(0, input_str.find(delimiter));
        int target_position = std::stoi(input_str.substr(input_str.find(delimiter)+1,input_str.size() - input_str.find(delimiter) - 1));

        servo_collection.set_verbose(true);
        servo_collection.write(servo_name,"Position",target_position);
        servo_collection.set_verbose(false);
    }

    servo_collection.set_verbose(true);
    servo_collection.require_servo_state(asp::ServoStates::SwitchOnDisabled);
    servo_collection.set_verbose(false);  
    servo_collection.disconnect();

    return 0;
}

