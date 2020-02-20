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
	string source;
    chrono::system_clock::time_point time; //ctime doesn't show less than second
    bool broken;
};

 class widget {
 private:
     widget_def w1;

 public: 

    widget(string producer, int id_length, bool broken_widget) {
        //Define ID as 16-16 random characters
        string temp;
        char randLetNum;
        int LetOrNum, IDSeperator = id_length/2 - 1; //Add '-' halfway in the ID

        for (int i = 0; i < id_length; i++) {
            LetOrNum = rand() % 2;
            if (LetOrNum == 0) {
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
        this->w1.source = producer;
        //Define time
        this->w1.time = chrono::system_clock::now();         
        //Decide if this widget is broken
        this->w1.broken = broken_widget;
    }

    string print_id() {
        return (this->w1).id;
    }

    string print_producer() {
        return (this->w1).source;
    }

    string print_time() {
        struct tm currTime; //struct zero-initializes currTime
        time_t timeCurrent = chrono::system_clock::to_time_t((this->w1).time);
        localtime_s(&currTime, &timeCurrent); //convert time since epoch to local time
        string time = to_string(currTime.tm_hour) + ':' + to_string(currTime.tm_min) + ':' + to_string(currTime.tm_sec);
        return time;
    }

    string print_broken() {
        if (this->w1.broken) {
            return "true";
        }
        else {
            return "false";
        }
    }

    chrono::duration<double> time_consume(chrono::system_clock::time_point time_consumed) {
        return time_consumed - (this->w1.time);
    }
 };

void produce_widget(vector<widget*>&, string, int, int, int&, int);
void consume_widget(vector<widget*>&,string);

mutex mtx;

int main()
{
    srand(time(NULL)); //random number/letter generator

    int idLength = 32;
    int numProducers = 1;
    int numConsumers = 1;
    int maxWidgets = 50;
    int brokenWidget = 1;
    int widgetNum = 0;
    vector<widget*> createdWidgets;
    createdWidgets.reserve(maxWidgets);
    vector<thread> tProducers;
    vector<thread> tConsumers;

    for (int i = 0; i < numProducers; i++) {
        string producerNum = "producer_" + to_string(i);
        thread t_p(produce_widget, ref(createdWidgets), producerNum, idLength, brokenWidget, ref(widgetNum), maxWidgets);
        tProducers.push_back(move(t_p));
    }
    
    for (int i = 0; i < numConsumers;i++) {
        string consumerNum = "consumer_" + to_string(i);
        thread t_c(consume_widget, ref(createdWidgets), consumerNum);
        tConsumers.push_back(move(t_c));
    }
    
    //After we consume the broken widget we should join all the threads end the program 
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

void produce_widget(vector<widget*> &createdWidgets, string producer,int idLength, int brokenWidget, int &widgetNum, int maxWidgets) {
    while (widgetNum < maxWidgets) {
        createdWidgets.push_back(new widget(producer, idLength, widgetNum == brokenWidget));
        widgetNum++;
    }
}

void consume_widget(vector<widget*> &createdWidgets, string consumer) {
    
    bool brokenWidget = false;

    do {

        if (createdWidgets.empty()) {
            continue;
        }
        //Take the widget out of the vector when consuming it (so the same widget doesn't get consumed)
        widget* w = createdWidgets.back();
        createdWidgets.pop_back();

        string widgetOutput = "[id=" + w->print_id() +
            " source=" + w->print_producer() +
            " time=" + w->print_time() +
            " broken=" + w->print_broken() + "]";

        if (w->print_broken() == "false") {
            //lock the output so they don't get scrambled with other consumers
            mtx.lock();
            cout << consumer << " consumes " << widgetOutput << " in " 
                << w->time_consume(chrono::system_clock::now()).count() << "s time\n";
            mtx.unlock();
        }
        else {
            brokenWidget = true;
            cout << consumer << " found a broken widget " << widgetOutput << " -- stopping production\n";
        }
    } while (!brokenWidget);

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
