#ifndef WIDGET_DEFINITION_H //Always good practice to have headers to protect multiple instaces
#define WIDGET_DEFINITION_H

#include <string.h>
#include <chrono>
#include <thread>

#define NanosecondsInHour 3600000000000 

class widget {
private:
    std::string id;
    std::string producer;
    std::chrono::system_clock::time_point timeCreated; //ctime doesn't show less than second
    bool isBroken;

public:

    widget(std::string producer, int id_length, bool isBroken) {
        //Define ID as 16-16 random characters
        char randLetNum;
        int IDSeperator = id_length / 2 - 1; //Add '-' halfway in the ID
        bool LetOrNum;

        for (int i = 0; i < id_length; i++) {
            LetOrNum = static_cast<bool>(rand() % 2);
            if (LetOrNum) {
                randLetNum = '0' + rand() % 10; //Random number
            }
            else {
                randLetNum = 'a' + rand() % 26; //Random letter
            }
            this->id += randLetNum;
            //Add - in ID
            if (i == IDSeperator) {
                this->id += '-';
            }
        }
        //Define source
        this->producer = producer;
        //Define time that widget was created
        this->timeCreated = std::chrono::system_clock::now();
        //Decide if this widget is broken
        this->isBroken = isBroken;
    }

    std::string get_id() {
        return this->id;
    }

    std::string get_producer() {
        return this->producer;
    }

    std::chrono::system_clock::time_point get_time_created(){
        return this->timeCreated;
    }
    
    std::string print_time_created() {
        //time_t timeCreatedEpoch = std::chrono::system_clock::to_time_t(this->timeCreated);
	//struct tm* timeCreatedLocal = std::localtime(&timeCreatedEpoch); //convert [time since epoch] to [local time]
        //return (std::to_string(timeCreatedLocal->tm_hour) + ':' + std::to_string(timeCreatedLocal->tm_min) + ':' + std::to_string(timeCreatedLocal->tm_sec));
	std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(this->timeCreated.time_since_epoch());
        //All I want is the past hour
	ns = ns % NanosecondsInHour;	
	return std::to_string(ns.count());
    }

    //TODO:GET RID OF THE print true/false (one function to print and one to return the value)
    bool get_is_broken() {
        return this->isBroken;	    
    }
    
    std::string print_is_broken() {
        if (this->isBroken) {
            return "true";
        }
        else {
            return "false";
        }
    }

    std::chrono::duration<double> get_time_duration(std::chrono::system_clock::time_point timeConsumed) {
        return (timeConsumed - (this->timeCreated));
    }
};

#endif // !WIDGET_DEFINITION_H
