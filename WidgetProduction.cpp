// WidgetProduction.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

using namespace std;

 struct widget_def{
    string id;
    string producer;
    chrono::system_clock::time_point timeInit; //ctime doesn't show less than second
    bool isBroken;
};

 class widget {
 private:
     widget_def w1;

 public: 

    widget(string producer, int id_length, bool broken_widget) {
        //Define ID as 16-16 random characters
        char randLetNum;
        int IDSeperator = id_length/2 - 1; //Add '-' halfway in the ID
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
        //Define time
        this->w1.timeInit = chrono::system_clock::now();         
        //Decide if this widget is broken
        this->w1.isBroken = broken_widget;
    }

    string get_id() {
        return (this->w1).id;
    }

    string get_producer() {
        return (this->w1).producer;
    }

    //Get time in string format HH:MM:SS to output
    string get_time() {
        struct tm currTime = {}; //zero-initializes currTime
        time_t timeCurrent = chrono::system_clock::to_time_t((this->w1).timeInit);
        localtime_s(&currTime, &timeCurrent); //convert [time since epoch] to [local time]
        string time = to_string(currTime.tm_hour) + ':' + to_string(currTime.tm_min) + ':' + to_string(currTime.tm_sec);
        return time;
    }

    string get_broken() {
        if (this->w1.isBroken) {
            return "true";
        }
        else {
            return "false";
        }
    }

    chrono::duration<double> get_time_consumed(chrono::system_clock::time_point time_consumed) {
        return time_consumed - (this->w1.timeInit);
    }
 };

void produce_widget(vector<widget*>&, string, int, int, int&, int);
void consume_widget(vector<widget*>&,string, bool&);

mutex mtx_produce;
mutex mtx_consume;
mutex mtx_print;
mutex mtx_broken;

int main()
{
    //TODO: Since I am running multiple functions the same second I need more fine grain on time
    srand(time(NULL)); //random number/letter generator

    int widgetIdLength = 32;
    int numProducers = 10; //Right now can only have 2 producers max hmmm
    int numConsumers = 5;
    int maxWidgets = 50;
    int brokenWidget = 2; //Which produced widget should be broken
    int totWidgetsCreated = 0;
    bool isBrokenWidget = false;
    vector<widget*> createdWidgets;
    createdWidgets.reserve(maxWidgets);
    vector<thread> tProducers;
    vector<thread> tConsumers;

    for (int i = 0; i < numProducers; i++) {
        string producerNum = "producer_" + to_string(i);
        thread t_p(produce_widget, ref(createdWidgets), producerNum, widgetIdLength, brokenWidget, ref(totWidgetsCreated), maxWidgets);
        tProducers.push_back(move(t_p));
    }
    
    for (int i = 0; i < numConsumers;i++) {
        string consumerNum = "consumer_" + to_string(i);
        thread t_c(consume_widget, ref(createdWidgets), consumerNum, ref(isBrokenWidget));
        tConsumers.push_back(move(t_c));
    }
    
    //After we consume the broken widget we should join all the threads and end the program 
    for (auto it = tProducers.begin(); it != tProducers.end(); ++it) {
        (*it).join();
    }
    for (auto it = tConsumers.begin(); it != tConsumers.end(); ++it) {
        (*it).join();
    }

    //Free the pointers in the vector
    for (auto i = createdWidgets.begin(); i != createdWidgets.end(); i++) {
        delete *i;
    }

}

void produce_widget(vector<widget*> &createdWidgets, string producer,int idLength, int brokenWidget, int &totWidgetsCreated, int maxWidgets) {
    while (totWidgetsCreated < maxWidgets) {
        mtx_produce.lock();
        createdWidgets.push_back(new widget(producer, idLength, totWidgetsCreated == brokenWidget));
        totWidgetsCreated++;
        mtx_produce.unlock();
        //Add a delay to let other producers get a chance to create widgets
        this_thread::sleep_for(chrono::nanoseconds(1000)); //This seems to work for now anything lower and it is not a good enough delay
    }
}

void consume_widget(vector<widget*> &createdWidgets, string consumer, bool &isBrokenWidget) {
    
    while(!isBrokenWidget){
        
        //Check that there are widgets for the consumer to consume
        mtx_consume.lock();
        if (createdWidgets.empty()) {
            mtx_consume.unlock(); //Make sure to unlock since I will be continuing here
            continue;
        }
        //Take the widget out of the vector when consuming it (so the same widget doesn't get consumed)
        widget w = *createdWidgets.back();
        createdWidgets.pop_back();
        mtx_consume.unlock();

        string widgetOutput = "[id=" + w.get_id() +
            " source=" + w.get_producer() +
            " time=" + w.get_time() +
            " broken=" + w.get_broken() + "]";

        if (w.get_broken() == "false") {
            //lock the output so they don't get scrambled with other consumers
            mtx_print.lock();
            cout << consumer << " consumes " << widgetOutput << " in " 
                << w.get_time_consumed(chrono::system_clock::now()).count() << "s time\n";
            mtx_print.unlock();
        }
        else {
            mtx_broken.lock();
            isBrokenWidget = true;
            mtx_broken.unlock();
            //Need to lock print too otherwise broken widget print can be inside the unbroken widget print
            mtx_print.lock();
            cout << consumer << " found a broken widget " << widgetOutput << " -- stopping production\n";
            mtx_print.unlock();
        }
    }

}