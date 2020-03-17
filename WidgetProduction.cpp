// WidgetProduction.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <mutex>
#include <vector>
#include <ctime>
#include <atomic>
#include "widget_definition.h"
#include <pthread.h>

void produce_widget(std::vector<widget*>& createdWidgets, std::string producer, int idLength, int brokenWidget, int& totalWidgetsCreated, int maxWidgets, int threadNum);
void consume_widget(std::vector<widget*>& createdWidgets, std::string consumer, bool& consumedBrokenWidget);

//Use rwlocks to protect data
pthread_rwlock_t widgetRWLock;
pthread_rwlock_t printRWLock;
pthread_rwlock_t brokenRWLock;

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

    int totalWidgetsCreated = 0;
    const int widgetIdLength = 32;
    //TODO: protect isBrokenWidget using atomic type
    bool consumedBrokenWidget = false;
    //widget pointer to store all the widgets created
    std::vector<widget*> createdWidgets;
    createdWidgets.reserve(maxWidgets);
    std::vector<std::thread> Producers;
    std::vector<std::thread> Consumers;

    //Initialize rwlocks to default
    pthread_rwlock_init(&widgetRWLock, NULL);
    pthread_rwlock_init(&printRWLock, NULL);
    pthread_rwlock_init(&brokenRWLock, NULL);

    for (int i = 0; i < totalProducers; i++) {
        std::string producerName = "producer_" + std::to_string(i);
        std::thread tempProducer(produce_widget, std::ref(createdWidgets), producerName, widgetIdLength, brokenWidget, std::ref(totalWidgetsCreated), maxWidgets, i);
        //std::move() here allows the tempProducer thread to move because copy function for threads disabled
        Producers.push_back(std::move(tempProducer));
    }
    
    for (int i = 0; i < totalConsumers;i++) {
        std::string consumerName = "consumer_" + std::to_string(i);
        std::thread tempConsumer(consume_widget, std::ref(createdWidgets), consumerName, std::ref(consumedBrokenWidget));
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

void produce_widget(std::vector<widget*> &createdWidgets, std::string producer,int idLength, int brokenWidget, int &totalWidgetsCreated, int maxWidgets, int threadNum) {
    pthread_rwlock_rdlock(&widgetRWLock);
    int localTotalWidgetsCreated = totalWidgetsCreated;
    pthread_rwlock_unlock(&widgetRWLock);

    while (localTotalWidgetsCreated < maxWidgets) {
	//Define the srand() when producing the widget in order to get different random numbers for each thread!
	//TODO:Check the output of the ns.count() to check threads have same ns! (created at different times)
	std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
        srand(ns.count() + threadNum); //random number/letter generator for each thread

        pthread_rwlock_wrlock(&widgetRWLock);
	localTotalWidgetsCreated = totalWidgetsCreated;
	if(localTotalWidgetsCreated >= maxWidgets){
            pthread_rwlock_unlock(&widgetRWLock);
            break;
	}
	createdWidgets.push_back(new widget(producer, idLength, totalWidgetsCreated == brokenWidget));
        totalWidgetsCreated++; //Need to update both the creation of the widget and the number under the same mutex
        pthread_rwlock_unlock(&widgetRWLock);    

               //Add a delay to let other producers get a chance to create widgets
        //TODO: fix this so don't have to use a sleep function to wait for the threads to finish (yield?)
       	std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
    }
}

void consume_widget(std::vector<widget*> &createdWidgets, std::string consumer, bool &consumedBrokenWidget) {
    //protect isbrokenwidget in the while loop
    pthread_rwlock_rdlock(&brokenRWLock);
    bool isBroken = consumedBrokenWidget;
    pthread_rwlock_unlock(&brokenRWLock);

    while(!isBroken){
	//Update this isBroken
	pthread_rwlock_rdlock(&brokenRWLock);
        isBroken = consumedBrokenWidget;
	pthread_rwlock_unlock(&brokenRWLock);
        
	//Check that there are widgets for the consumer to consume
        pthread_rwlock_wrlock(&widgetRWLock);
	//Need to check to make sure isn't broken as well
        if (createdWidgets.empty() || isBroken) {
            pthread_rwlock_unlock(&widgetRWLock); //Unlock for other consumeris
	    //TODO: Replace continue with goto!!!
            continue;
        }
        //Take the widget out of the vector when consuming it (so the same widget doesn't get consumed twice)
        widget w = *createdWidgets.back();
        createdWidgets.pop_back();
	pthread_rwlock_unlock(&widgetRWLock);

        std::string widgetOutput = "[id=" + w.get_id() +
            " source=" + w.get_producer() +
            " time=" + w.print_time_created() +
            " broken=" + w.print_is_broken() + "]";

	//TODO:FIX THE get_is_broken and print_is_broken
        if (!w.get_is_broken()) {
	    pthread_rwlock_wrlock(&printRWLock);
            //Need to convert the time to string otherwise the time will be off and the newline won't print 
            //so some lines might combine
            std::cout << consumer << " consumes " << widgetOutput << " in "
                << std::to_string(w.get_time_duration(std::chrono::system_clock::now()).count()) << "s time\n";
            //std::this_thread::sleep_for(std::chrono::nanoseconds(1000)); This causes the program to consume the first widget multiple times??
            pthread_rwlock_unlock(&printRWLock);
        }
        else {
	    //TODO:possible don't need the writelock here as nothing is changing this to FALSE!!!! at the same time and only ONE broken ediget
            pthread_rwlock_wrlock(&brokenRWLock);
            consumedBrokenWidget = true;
	    pthread_rwlock_unlock(&brokenRWLock);

	    //TODO:goto to this area right here
            pthread_rwlock_rdlock(&brokenRWLock);
	    isBroken = consumedBrokenWidget; //need to update brokenProtect as well to break the loop
            pthread_rwlock_unlock(&brokenRWLock);
	    //Need to lock print too otherwise can print consumes and broken widget at same time
	    pthread_rwlock_wrlock(&printRWLock);
            std::cout << consumer << " found a broken widget " << widgetOutput << " -- stopping production\n";
            pthread_rwlock_unlock(&printRWLock);
        }
    }

}
