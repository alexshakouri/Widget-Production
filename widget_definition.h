#ifndef WIDGET_DEFINITION_H //Always good practice to have headers to protect multiple instaces
#define WIDGET_DEFINITION_H

#include <string.h>
#include <chrono>
#include <thread>

#define NanosecondsPerDay 86400000000000
#define NanosecondsPerHour (NanosecondsPerDay/24)
#define NanosecondsPerMinute (NanosecondsPerHour/60)
#define NanosecondsPerSecond (NanosecondsPerMinute/60)

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
	std::chrono::microseconds ns = std::chrono::duration_cast<std::chrono::microseconds>(this->timeCreated.time_since_epoch());
        //hour:min:sec.microsecond
	ns = ns % NanosecondsPerDay;
	int nsHour = ns.count() / NanosecondsPerHour;
        ns = ns % NanosecondsPerHour;
        int nsMinute = ns.count() / NanosecondsPerMinute;
	ns = ns % NanosecondsPerMinute;
	int nsSecond = ns.count() / NanosecondsPerSecond;
	ns = ns % NanosecondsPerSecond;
        	
	return (std::to_string(nsHour) + ":" + std::to_string(nsMinute) + ":" + std::to_string(nsSecond) + "." + std::to_string(ns.count()));
    }

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
