#include <iostream>
#include <fstream>
#include <pthread.h>
#include <chrono>
#include <thread>


using namespace std;

pthread_mutex_t output_operation, atm[10], customer_ready[10], atm_ready[10], bill[5]; // Mutex locks have been created
int cableTV, electricity, gas, telecommunication, water; // Global variables which keep total amount of one bill type

struct customer_data{ // a struct which holds the information about a customer
    int id, sleep_time, atm_instance, amount;
    string bill_type;

};

struct atm_data{ // a struct which keeps the information an atm thread needs
    int id;
    string myoutputfile;

};

struct customer_data atm_check[10]; // global data for each atm. Each element in the array holds the information of the next customer which will be processed for that atm.

void *customer_runner(void *param); // each customer thread runs this function
void *atm_runner(void *param); // each atm thread runs this function

int main(int argc, char* argv[] ) {

    string potential_outputfilename = argv[1]; // name of the input file is kept for creation of the name of output file

    string line, bill_type; // temp input variables
    ifstream myfile(argv[1]); // input file is open
    ofstream myoutputfile(potential_outputfilename.substr(0, potential_outputfilename.find("."))+"_log.txt"); // output file is open with the requested name

    int numberOfCustomer, sleep_time, atm_instance, amount; // temp input variables
    myfile >> numberOfCustomer; // read number of customer from the input file

    pthread_t atm_threads[10]; // atm thread array is created
    pthread_t customer_threads[numberOfCustomer]; // customer thread array is created
    struct customer_data cust_data[numberOfCustomer]; // data of each customer array is created
    struct atm_data atm_data[10]; // data of each atm array is created

    output_operation = PTHREAD_MUTEX_INITIALIZER; // mutex is initialized to 1, which means it can be locked. This mutex lock allows just one thread to write to the output file at one time.

    for(int i=0;i<5;i++){

        bill[i] = PTHREAD_MUTEX_INITIALIZER; // these mutex locks are initialized to 1, which means they can be locked. These mutex locks allow just one thread to change the amount of a bill type at one time.
    }


    for(int i=0;i < 10;i++){

        atm[i] = PTHREAD_MUTEX_INITIALIZER; // these mutex locks allow just one customer to be in critical region for each atm and change the global data of one atm. Also it enables to occur FIFO queue for customers coming when atm is busy.
        customer_ready[i] = PTHREAD_MUTEX_INITIALIZER; // these mutex locks become 1 when customers are ready for process.
        pthread_mutex_lock(&customer_ready[i]); // they start from 0 because no customer is ready at this stage
        atm_ready[i] = PTHREAD_MUTEX_INITIALIZER; // these mutex locks become 1 when atms are ready for process.
        pthread_mutex_lock(&atm_ready[i]); // they start from 0 because no atm is ready at this stage

        atm_data[i].id = i; // atm knows its id
        atm_data[i].myoutputfile = potential_outputfilename.substr(0, potential_outputfilename.find("."))+"_log.txt"; // atm knows name of the output file

        pthread_create(&atm_threads[i], NULL, atm_runner, (void *)&atm_data[i]); // atm threads are created

    }

    for(int i=0;i < numberOfCustomer;i++){

        myfile>>line; // input file is read

        sleep_time = stoi(line.substr(0, line.find(","))); // sleep time of a customer is extracted
        line = line.substr(line.find(",")+1); // sleep time is discarded
        atm_instance = stoi(line.substr(0, line.find(","))); // atm instance of a customer is extracted
        line = line.substr(line.find(",")+1); // atm instance is discarded
        bill_type = line.substr(0, line.find(",")); // bill type of a customer is extracted
        line = line.substr(line.find(",")+1); // bill type is discarded
        amount = stoi(line); // bill amount of a customer is extracted

        cust_data[i].id = i+1; // customer knows its id
        cust_data[i].sleep_time = sleep_time; // customer knows its sleep time
        cust_data[i].atm_instance = atm_instance-1; // customer knows which atm he/she will operate
        cust_data[i].bill_type = bill_type; // customer knows which type of bill he/she will pay
        cust_data[i].amount = amount; // customer knows the bill amount

    }

    
    for(int i=0;i < numberOfCustomer;i++){

        pthread_create(&customer_threads[i], NULL, customer_runner, (void *)&cust_data[i]); // customer threads are created. Customer threads are created in a different loop (than previous for loop) in order to reduce the creation time difference between customer threads.

    }


    for(int i=0;i<numberOfCustomer;i++){

        pthread_join(customer_threads[i], NULL); // main threads waits for end of customer threads
    }

    myoutputfile.close(); // output file is closed



    ofstream myoutputfile2(potential_outputfilename.substr(0, potential_outputfilename.find("."))+"_log.txt", std::ios_base::app); // output file is open for append

    myoutputfile2<<"All payments are completed."<<endl; // Last information is printed
    myoutputfile2<<"CableTV: "<<cableTV<<"TL"<<endl;
    myoutputfile2<<"Electricity: "<<electricity<<"TL"<<endl;
    myoutputfile2<<"Gas: "<<gas<<"TL"<<endl;
    myoutputfile2<<"Telecommunication: "<<telecommunication<<"TL"<<endl;
    myoutputfile2<<"Water: "<<water<<"TL"<<endl;

    myoutputfile2.close(); // output file is closed

    return 0;
}

void *customer_runner(void *param){ // customer thread function

    struct customer_data *cust_data;

    cust_data = (struct customer_data *) param; // customer data is taken from the data send from main

    std::this_thread::sleep_for(std::chrono::milliseconds(cust_data->sleep_time)); // customer threads sleep for their given sleep times

    pthread_mutex_lock(&atm[cust_data->atm_instance]); // customer threads occupy an atm instance

    atm_check[cust_data->atm_instance].id = cust_data->id; // customer threads change the global data of that atm instance
    atm_check[cust_data->atm_instance].amount = cust_data->amount; // customer threads change the global data of that atm instance
    atm_check[cust_data->atm_instance].bill_type = cust_data->bill_type; // customer threads change the global data of that atm instance


    pthread_mutex_unlock(&customer_ready[cust_data->atm_instance]); // customer threads say that I am ready for process

    pthread_mutex_lock(&atm_ready[cust_data->atm_instance]); // customer threads wait for atm thread to finish their execution

    pthread_mutex_unlock(&atm[cust_data->atm_instance]); // customers let other customers to get into the critical region

}

void *atm_runner(void *param){

    struct atm_data *atmdata;

    atmdata = (struct atm_data *) param; // customer data is taken from the data send from main

    ofstream myoutputfile(atmdata->myoutputfile, std::ios_base::app); // output file is open for each atm thread in append mode

    while(1){ // it will always look until program finishes

        pthread_mutex_lock(&customer_ready[atmdata->id]); // atm instances wait for a customer to be ready



        if (atm_check[atmdata->id].bill_type == "cableTV"){ // check for bill type of customer
            pthread_mutex_lock(&bill[0]); // only one atm thread can change the cableTV variable
            cableTV += atm_check[atmdata->id].amount; // amount of bill is added to global sum of that type of bill
            pthread_mutex_unlock(&bill[0]);
        }

        else if (atm_check[atmdata->id].bill_type == "electricity"){ // check for bill type of customer
            pthread_mutex_lock(&bill[1]);// only one atm thread can change the electricity variable
            electricity += atm_check[atmdata->id].amount; // amount of bill is added to global sum of that type of bill
            pthread_mutex_unlock(&bill[1]);
        }

        else if (atm_check[atmdata->id].bill_type == "gas"){ // check for bill type of customer
            pthread_mutex_lock(&bill[2]); // only one atm thread can change the gas variable
            gas += atm_check[atmdata->id].amount; // amount of bill is added to global sum of that type of bill
            pthread_mutex_unlock(&bill[2]);
        }

        else if (atm_check[atmdata->id].bill_type == "telecommunication"){ // check for bill type of customer
            pthread_mutex_lock(&bill[3]); // only one atm thread can change the telecommunication variable
            telecommunication += atm_check[atmdata->id].amount; // amount of bill is added to global sum of that type of bill
            pthread_mutex_unlock(&bill[3]);
        }

        else if (atm_check[atmdata->id].bill_type == "water"){ // check for bill type of customer
            pthread_mutex_lock(&bill[4]); // only one atm thread can change the water variable
            water += atm_check[atmdata->id].amount; // amount of bill is added to global sum of that type of bill
            pthread_mutex_unlock(&bill[4]);
        }


        pthread_mutex_lock(&output_operation); // only one thread can write to the output file at one time

        myoutputfile<<"Customer"<<atm_check[atmdata->id].id<<","<<atm_check[atmdata->id].amount<<"TL,"<<atm_check[atmdata->id].bill_type<<endl; // atm threads write the payments to the output file

        pthread_mutex_unlock(&output_operation);

        pthread_mutex_unlock(&atm_ready[atmdata->id]); // atm says that I am done and I am ready for next process

    }

}