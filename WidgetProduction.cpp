// WidgetProduction.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <mutex>
#include <vector>
#include <ctime>
#include <sys/timeb.h>
#include <atomic>
#include "widget_definition.h"

//be better to include varnames here in parameters to let the reader kn ow what each function does!
void produce_widget(std::vector<widget*>& createdWidgets, std::string producer, int idLength, int brokenWidget, int& totWidgetsCreated, int maxWidgets, int threadNum);
void consume_widget(std::vector<widget*>& createdWidgets, std::string consumer, bool& consumeBrokenWidget);

//Protect my resources!!
std::mutex gMTXWidgets;
std::mutex gMTXPrint;
std::mutex gMTXBroken;

#define gWidgetIdLength 32
#define defaultTotalProducers 1
#define defaultTotalConsumers 1
#define defaultMaxWidgets 11
#define defaultBrokenWidget 1

int main(int argc, char** args)
{
    int totalProducers = defaultTotalProducers;
    int totalConsumers = defaultTotalConsumers;
    int maxWidgets = defaultMaxWidgets;
    int brokenWidget = defaultBrokenWidget; //Which produced widget should be broken

    //Go through all the defined command line arguments
    //TODO: fix user input protection
    for (int i = 1; i < argc; i++) {
        if (strcmp(args[i], "-n") == 0) {
            i++;
            if (atoi(args[i]) != 0) {
                maxWidgets = atoi(args[i]);
            }
            else {
                std::cout << "Unable to read maximum widgets defaulting to " << (defaultMaxWidgets - 1)<< std::endl;
            }
        }
        else if (strcmp(args[i], "-p") == 0) {
            i++;
            if (atoi(args[i]) != 0) {
                totalProducers = atoi(args[i]);
            }
            else {
                std::cout << "Unable to read total producers defaulting to " << defaultTotalProducers << std::endl;
            }
        }
        else if (strcmp(args[i], "-c") == 0) {
            i++;
            if (atoi(args[i]) != 0) {
                totalConsumers = atoi(args[i]);
            }
            else {
                std::cout << "Unable to read total consumers defaulting to " << defaultTotalConsumers << std::endl;
            }
        }
        else if (strcmp(args[i], "-k") == 0) {
            i++;
            if (atoi(args[i]) != 0) {
                brokenWidget = atoi(args[i]);
            }
            else {
                std::cout << "Unable to read k-th broken widget defaulting to " << defaultBrokenWidget << std::endl;
            }
        }
        else {
            std::cout << "Do not recognize command " << args[i] << " please re-run" << std::endl;
            return 1;
        }
    }

    int totWidgetsCreated = 0;
    //TODO: protect isBrokenWidget using atomic type
    bool consumeBrokenWidget = false;
    //widget pointer to store all the widgets created
    std::vector<widget*> createdWidgets;
    createdWidgets.reserve(maxWidgets);
    std::vector<std::thread> Producers;
    std::vector<std::thread> Consumers;

    for (int i = 0; i < totalProducers; i++) {
        
        std::string producerNum = "producer_" + std::to_string(i);
        std::thread tempProducer(produce_widget, std::ref(createdWidgets), producerNum, gWidgetIdLength, brokenWidget, std::ref(totWidgetsCreated), maxWidgets, i);
        //std::move() here allows the tempProducer thread to move because copy function for threads disabled
        Producers.push_back(std::move(tempProducer));
    }
    
    for (int i = 0; i < totalConsumers;i++) {
        std::string consumerNum = "consumer_" + std::to_string(i);
        std::thread tempConsumer(consume_widget, std::ref(createdWidgets), consumerNum, std::ref(consumeBrokenWidget));
        Consumers.push_back(std::move(tempConsumer));
    }
    
    //After we consume the broken widget we should join all the threads and end the program
    for (auto iterator = Producers.begin(); iterator != Producers.end(); ++iterator) {
        (*iterator).join();
    }
    for (auto iterator = Consumers.begin(); iterator != Consumers.end(); ++iterator) {
        (*iterator).join();
    }

    //Free the pointers in the widgets vector, the vectors free themselves 
    for (auto iterator = createdWidgets.begin(); iterator != createdWidgets.end(); iterator++) {
        delete *iterator;
    }

}

void produce_widget(std::vector<widget*> &createdWidgets, std::string producer,int idLength, int brokenWidget, int &totWidgetsCreated, int maxWidgets, int threadNum) {
    while (totWidgetsCreated < maxWidgets) {
        gMTXWidgets.lock();
        //Define the srand() when producing the widget in order to get different times for each thread!
        std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
        srand(ns.count() + threadNum); //random number/letter generator
        createdWidgets.push_back(new widget(producer, idLength, totWidgetsCreated == brokenWidget));
        totWidgetsCreated++;
        gMTXWidgets.unlock();
        //Add a delay to let other producers get a chance to create widgets
        //TODO: fix this so don't have to use a sleep function to wait for the threads to finish
        std::this_thread::sleep_for(std::chrono::nanoseconds(1000)); //This seems to work for now anything lower and one thread takes over
    }
}

void consume_widget(std::vector<widget*> &createdWidgets, std::string consumer, bool &consumeBrokenWidget) {
    //protect isbrokenwidget in the while loop
    gMTXBroken.lock();
    bool brokenProtect = consumeBrokenWidget;
    gMTXBroken.unlock();

    while(!brokenProtect){
        
        //Check that there are widgets for the consumer to consume
        gMTXWidgets.lock();
        if (createdWidgets.empty()) {
            gMTXWidgets.unlock(); //Make sure to unlock to free other consumers
            continue;
        }
        //Take the widget out of the vector when consuming it (so the same widget doesn't get consumed twice)
        widget w = *createdWidgets.back();
        createdWidgets.pop_back();
        gMTXWidgets.unlock();

        std::string widgetOutput = "[id=" + w.get_id() +
            " source=" + w.get_producer() +
            " time=" + w.get_time_created() +
            " broken=" + w.get_is_broken() + "]";

        if (w.get_is_broken() == "false") {
            gMTXPrint.lock();
            //Need to convert the time to string otherwise the time will be off and the newline won't print 
            //so some lines might combine
            std::cout << consumer << " consumes " << widgetOutput << " in "
                << std::to_string(w.get_time_duration(std::chrono::system_clock::now()).count()) << "s time\n";
            //std::this_thread::sleep_for(std::chrono::nanoseconds(1000)); This causes the program to consume the first widget multiple times??
            gMTXPrint.unlock();
        }
        else {
            gMTXBroken.lock();
            consumeBrokenWidget = true;
            gMTXBroken.unlock();
            //Need to lock print too otherwise broken widget print can be inside the unbroken widget print
            gMTXPrint.lock();
            std::cout << consumer << " found a broken widget " << widgetOutput << " -- stopping production\n";
            gMTXPrint.unlock();
        }
        gMTXBroken.lock();
        brokenProtect = consumeBrokenWidget;
        gMTXBroken.unlock();
    }

}