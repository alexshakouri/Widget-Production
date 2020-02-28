#ifndef WIDGET_DEFINITION_H //Always good practice to have headers to protect multiple instaces
#define WIDGET_DEFINITION_H

#include <string>
#include <chrono>
#include <thread>

struct widget_def {
    std::string id;
    std::string producer;
    std::chrono::system_clock::time_point timeCreated; //ctime doesn't show less than second
    bool isBroken;
};

class widget {
private:
    widget_def w1;

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
            (this->w1).id += randLetNum;
            //Add - in ID
            if (i == IDSeperator) {
                (this->w1).id += '-';
            }
        }
        //Define source
        this->w1.producer = producer;
        //Define time that widget was created
        this->w1.timeCreated = std::chrono::system_clock::now();
        //Decide if this widget is broken
        this->w1.isBroken = isBroken;
    }

    std::string get_id() {
        return (this->w1).id;
    }

    std::string get_producer() {
        return (this->w1).producer;
    }

    //Get time in string format HH:MM:SS to output to STDOUT
    //TODO: output the time in milliseconds to see a difference
    std::string get_time_created() {
        struct tm timeCreatedLocal = {}; //zero-initializes currTime
        time_t timeCreatedEpoch = std::chrono::system_clock::to_time_t((this->w1).timeCreated);
        localtime_s(&timeCreatedLocal, &timeCreatedEpoch); //convert [time since epoch] to [local time]
        return (std::to_string(timeCreatedLocal.tm_hour) + ':' + std::to_string(timeCreatedLocal.tm_min) + ':' + std::to_string(timeCreatedLocal.tm_sec));
    }

    std::string get_is_broken() {
        if (this->w1.isBroken) {
            return "true";
        }
        else {
            return "false";
        }
    }

    std::chrono::duration<double> get_time_duration(std::chrono::system_clock::time_point timeConsumed) {
        return (timeConsumed - (this->w1.timeCreated));
    }
};

#endif // !WIDGET_DEFINITION_H