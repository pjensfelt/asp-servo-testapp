#include <asp_servo_api/asp_servo.h>
#include <asp_servo_api/servo.h>
#include <iostream>
#include <chrono>
#include <thread>

int main() {

    // Loading servo settings from XML
    asp::ServoCollection servo_collection("../config/asp_test_torque.xml");

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
        int16_t target_torque = std::stoi(input_str.substr(input_str.find(delimiter)+1,input_str.size() - input_str.find(delimiter) - 1));
        if (target_torque > 500) {
            target_torque = 500;
        }
        if (target_torque < -500) {
            target_torque = -500;
        }
        servo_collection.set_verbose(true);
        // Writing torque to servo
        servo_collection.write(servo_name,"Torque",target_torque);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        servo_collection.set_verbose(false);
    }

    servo_collection.set_verbose(true);
    servo_collection.require_servo_state(asp::ServoStates::SwitchOnDisabled);
    servo_collection.set_verbose(false);  
    servo_collection.disconnect();

    return 0;
}

