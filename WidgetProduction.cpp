// WidgetProduction.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <mutex>
#include <vector>
#include <ctime>
#include <sys/timeb.h>
#include <atomic>
#include "widget_definition.h"


//TODO: get rid of that annoying print on the same line!!! '/n' is somehow missing in the print??

void produce_widget(std::vector<widget*>&, std::string, int, int, int&, int, int);
void consume_widget(std::vector<widget*>&, std::string, bool&);

//Protect my resources!!
std::mutex mtx_produce;
std::mutex mtx_consume;
std::mutex mtx_print;
std::mutex mtx_broken;

int main()
{
    int widgetIdLength = 32;
    int totProducers = 10; //Right now can only have 2 producers max hmmm
    int totConsumers = 5;
    int maxWidgets = 50;
    int brokenWidget = 2; //Which produced widget should be broken
    int totWidgetsCreated = 0;
    //TODO: protect isBrokenWidget using atomic type (didn't work for some reason)
    bool isBrokenWidget = false;

    //widget pointer to store all the widgets I create
    std::vector<widget*> createdWidgets;
    createdWidgets.reserve(maxWidgets);
    std::vector<std::thread> tProducers;
    std::vector<std::thread> tConsumers;

    for (int i = 0; i < totProducers; i++) {
        
        std::string producerNum = "producer_" + std::to_string(i);
        std::thread t_p(produce_widget, std::ref(createdWidgets), producerNum, widgetIdLength, brokenWidget, std::ref(totWidgetsCreated), maxWidgets, i);
        //move() here allows the t_p thread to move because copy function for threads disabled
        tProducers.push_back(std::move(t_p));
    }
    
    for (int i = 0; i < totConsumers;i++) {
        std::string consumerNum = "consumer_" + std::to_string(i);
        std::thread t_c(consume_widget, std::ref(createdWidgets), consumerNum, std::ref(isBrokenWidget));
        tConsumers.push_back(std::move(t_c));
    }
    
    //After we consume the broken widget we should join all the threads and end the program 
    for (auto it = tProducers.begin(); it != tProducers.end(); ++it) {
        (*it).join();
    }
    for (auto it = tConsumers.begin(); it != tConsumers.end(); ++it) {
        (*it).join();
    }

    //Free the pointers in the widgets vector
    //vectors will free themselves!
    for (auto i = createdWidgets.begin(); i != createdWidgets.end(); i++) {
        delete *i;
    }

}

void produce_widget(std::vector<widget*> &createdWidgets, std::string producer,int idLength, int brokenWidget, int &totWidgetsCreated, int maxWidgets, int threadNum) {
    while (totWidgetsCreated < maxWidgets) {
        mtx_produce.lock();
        //Define the srand() when producing the widget in order to get different times for each thread!
        std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
        srand(ns.count() + threadNum); //random number/letter generator
        createdWidgets.push_back(new widget(producer, idLength, totWidgetsCreated == brokenWidget));
        totWidgetsCreated++;
        mtx_produce.unlock();
        //Add a delay to let other producers get a chance to create widgets
        std::this_thread::sleep_for(std::chrono::nanoseconds(1000)); //This seems to work for now anything lower and one thread takes over
    }
}

void consume_widget(std::vector<widget*> &createdWidgets, std::string consumer, bool &isBrokenWidget) {
    
    while(!isBrokenWidget){
        
        //Check that there are widgets for the consumer to consume
        mtx_consume.lock();
        if (createdWidgets.empty()) {
            mtx_consume.unlock(); //Make sure to unlock to free other consumers
            continue;
        }
        //Take the widget out of the vector when consuming it (so the same widget doesn't get consumed twice)
        widget w = *createdWidgets.back();
        createdWidgets.pop_back();
        mtx_consume.unlock();

        std::string widgetOutput = "[id=" + w.get_id() +
            " source=" + w.get_producer() +
            " time=" + w.get_time_created() +
            " broken=" + w.get_is_broken() + "]";

        if (w.get_is_broken() == "false") {
            //lock the output so they don't get scrambled with other consumers
            mtx_print.lock();
            //I need to convert the time to string otherwise the time will be off and the newline won't print 
            //so some lines might combine
            std::cout << consumer << " consumes " << widgetOutput << " in "
                << std::to_string(w.get_time_duration(std::chrono::system_clock::now()).count()) << "s time\n";
            //std::this_thread::sleep_for(std::chrono::nanoseconds(1000)); This causes the program to consume the first widget multiple times??
            mtx_print.unlock();
        }
        else {
            //TODO: Think about combining broken mutex with the print mutex (atm this is best way I think)
            mtx_broken.lock(); 
            isBrokenWidget = true; 
            mtx_broken.unlock();
            //Need to lock print too otherwise broken widget print can be inside the unbroken widget print
            mtx_print.lock();
            std::cout << consumer << " found a broken widget " << widgetOutput << " -- stopping production\n";
            mtx_print.unlock();
        }
    }

}