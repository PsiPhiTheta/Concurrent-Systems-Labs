//Author: Thomas Hollis (9563426)
//Project: Concurrent Systems labs (University of Manchester)
//Year: 2017


//0.1 Inclusions (headers)_____________________________________________________________________________________
#include <iostream>
#include <thread>
#include <random>
#include <mutex>
#include <map>
#include <condition_variable>
#include <string>
#include <windows.h>


//0.2 Inclusions (namespaces)__________________________________________________________________________________
using namespace std;


//1. Global function declarations______________________________________________________________________________
int search();


//2. Global Variables__________________________________________________________________________________________
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); //Small piece of code used for coloured text

int const MAX_NUM_OF_CHAN = 6; //number of ADCInputChannels
int const MAX_NUM_OF_THREADS = 6; //number of threads
int const DATA_BLOCK_SIZE = 20; //number of readings per thread in receiver array
int const NUM_OF_LINKS = 3; //number of links for writing to the receiver

std::random_device rd; //random device
std::mt19937 gen(rd()); //Mersenne Twister pseudo-random generator
std::uniform_int_distribution<> dis(100,500); //Gaussian distributed int generator (between values of 100-500)

std::mutex mutexMAP; //map mutex
std::map<std::thread::id, int> threadIDs; //thread ID map

std::mutex mutexLOCK; //ADC lock mutex

std::mutex mutexADC; //ADC request mutex (condition variables / notify_all)
std::condition_variable cond_var; //ADC request condition variable

std::mutex mutexLINK; //link request mutex
std::condition_variable cond; //link request condition variable

//3. Classes___________________________________________________________________________________________________
class Receiver //stores data read by threads from ADC in a 2D array, accessible by Link (x3)
{
    public:
        Receiver () //initialise dataBlocks
        {
            for (int i = 0; i < MAX_NUM_OF_THREADS; i++)
            {
                for (int j = 0; j < DATA_BLOCK_SIZE; j++)
                {
                    dataBlocks[i][j] = 0;
                }
            }
        }

        void receiveDataBlock(int id, double data[]) //Receives a block of doubles and copies it in dataBlocks[][]
        {
            for(int i = 0; i < DATA_BLOCK_SIZE; i++)
            {
                dataBlocks[id][i] = data[i];
            }
        }

        void printBlocks() //prints out the receiver 2D array
        {
            cout << "The receiver contains the following data: " << endl;
            for(int i=0; i<(MAX_NUM_OF_THREADS-1); i++)
            {
                for(int j=0; j<(DATA_BLOCK_SIZE); j++)
                {
                    SetConsoleTextAttribute(hConsole, 11+i);
                    cout << dataBlocks[i][j]  << "   ";
                    SetConsoleTextAttribute(hConsole, 7);
                }
                cout << endl;
            }
            for(int j=0; j<(DATA_BLOCK_SIZE); j++) //needed for equal spacing with double digits
            {
                SetConsoleTextAttribute(hConsole, 9);
                cout << dataBlocks[(MAX_NUM_OF_THREADS-1)][j]  << "  ";
                SetConsoleTextAttribute(hConsole, 7);
            }
        }

    private:
        double dataBlocks[MAX_NUM_OF_THREADS][DATA_BLOCK_SIZE];
};

class Link //simulates connection between threads and Receiver object (via condition variable & wait/notify_all)
{
    public:
        Link (Receiver& r, int linkNum) : inUse(false), myReceiver(r), linkId(linkNum) {} //constructor

        bool isInUse() //check if the link is currently in use
        {
            return inUse;
        }

        void setInUse() //set the link status to busy
        {
            inUse = true;
        }

        void setIdle() //set the link status to idle
        {
            inUse = false;
        }

        void writeToDataLink(int id, double data[]) //write data[] to the receiver
        {
            myReceiver.receiveDataBlock(id, data);
        }

        int getLinkId() //returns the link Id
        {
            return linkId;
        }

    private:
        bool inUse;
        Receiver& myReceiver;  //Receiver reference
        int linkId;
};

class LinkAccessController //controller regulating thread access to individual links
{
    public:
        LinkAccessController(Receiver& r) : myReceiver(r), numOfAvailableLinks(NUM_OF_LINKS) //constructor
        {
            for (int i = 0; i < NUM_OF_LINKS; i++)
            {
                commsLinks.push_back(Link(myReceiver, i));   //initialises links
            }
        }

        Link requestLink() //request a comm's link: returns an available Link; if none are available, the calling thread is suspended.
        {
            int threadID = search(); //for later use in thread printing
            int linkNum; //variable containing the link number

            if(numOfAvailableLinks == 0) //if all links are busy
            {
                std::unique_lock <std::mutex> linkLock (mutexLINK);

                std::string output = "ERR: All links are busy, thread " + std::to_string(threadID) + " is about to suspend...\n";
                SetConsoleTextAttribute(hConsole, 7);
                cout << output;
                SetConsoleTextAttribute(hConsole, 7);

                while(numOfAvailableLinks == 0)
                {
                    cond.wait(linkLock);
                }
                return requestLink(); //loop back to requestLink function
            }
            else
            {
                if(!commsLinks[0].isInUse())
                {
                    linkNum = 0;
                    commsLinks[linkNum].setInUse();
                    numOfAvailableLinks = numOfAvailableLinks - 1; //update the value of available links

                    std::string output = "Link 0 accessed by thread " + std::to_string(threadID) + "\n";
                    SetConsoleTextAttribute(hConsole, 7);
                    cout << output;
                    SetConsoleTextAttribute(hConsole, 7);
                }
                else if(!commsLinks[1].isInUse())
                {
                    linkNum = 1;
                    commsLinks[linkNum].setInUse();
                    numOfAvailableLinks = numOfAvailableLinks - 1; //update the value of available links

                    std::string output = "Link 1 accessed by thread " + std::to_string(threadID) + "\n";
                    SetConsoleTextAttribute(hConsole, 7);
                    cout << output;
                    SetConsoleTextAttribute(hConsole, 7);
                }
                else if(!commsLinks[2].isInUse())
                {
                    linkNum = 2;
                    commsLinks[linkNum].setInUse();
                    numOfAvailableLinks = numOfAvailableLinks - 1; //update the value of available links

                    std::string output = "Link 2 accessed by thread " + std::to_string(threadID) + "\n";
                    SetConsoleTextAttribute(hConsole, 7);
                    cout << output;
                    SetConsoleTextAttribute(hConsole, 7);
                }
            }
            return commsLinks[linkNum];
        }

        void releaseLink(Link& releasedLink) //release a comms link:
        {
            std::unique_lock <std::mutex> linkLock (mutexLINK);

            int threadID = search(); //for later use in thread printing
            int linkID = releasedLink.getLinkId(); //get ID of the link

            std::string output = "Link " + std::to_string(linkID) + " released by thread " + std::to_string(threadID) + "\n";
            SetConsoleTextAttribute(hConsole, 7);
            cout << output;
            SetConsoleTextAttribute(hConsole, 7);

            commsLinks[linkID].setIdle(); //release the link
            numOfAvailableLinks = numOfAvailableLinks + 1; //update the value of available links

            cond.notify_all();  //communicate to the waiting threads to stop waiting
        }

    private:
        Receiver& myReceiver; //Receiver reference
        int numOfAvailableLinks;
        std::vector<Link> commsLinks;
};

class AdcInputChannel //simulates connection between threads and ADC object
{
    public:
        AdcInputChannel(int d) : currentSample(d) {} //constructor

        double getCurrentSample()  //used to request a sample from the sample channel
        {
            return 2*currentSample; //returns twice the channel value
        }

    private:
        int currentSample;
};

class Lock //regulates thread access to the ADC channels (via use of mutex & condvar wait/notify_all)
{
    public:
        Lock() : open(false) {} //constructor

        bool isLocked() //used to ensure ADC is not currently in use
        {
            return open;
        }

        void lock() //uses mutexADC to prevent other threads entering
        {
            open = 1;
            mutexLOCK.lock();
        }

        void unlock() //uses mutexADC to allow new threads to acquire ADC
        {
            open = 0;
            mutexLOCK.unlock();
        }

    private:
        bool open;
        std::mutex mutexLOCK;
};

class ADC //generates data (simulated), read by the threads through the AdcInputChannel (x6)
{
    public:
        ADC(std::vector<AdcInputChannel>& channels) : adcChannels(channels) {} //constructor

        void requestADC(int c) //allows or refuses access based on Lock object
        {
            if (!lock.isLocked()) //if ADC is not already locked by a thread
            {
                lock.lock();
                std::string output = "ADC locked by thread " + std::to_string(c) + "\n";
                SetConsoleTextAttribute(hConsole, 7);
                cout << output;
                SetConsoleTextAttribute(hConsole, 7);
            }
            else if (lock.isLocked()) //if ADC is already in use by another thread
            {
                std::unique_lock <std::mutex> lockADC (mutexADC);

                std::string output = "ERR: ADC is locked, thread " + std::to_string(c) + " is about to suspend...\n";
                SetConsoleTextAttribute(hConsole, 7);
                cout << output;
                SetConsoleTextAttribute(hConsole, 7);

                while(lock.isLocked()) //while the ADC is in use, thread set to wait for ADC to be released
                {
                    cond_var.wait(lockADC);
                }
                requestADC(c); //once ADC is released, loop back to request it
            }
        }

        double sampleADC(int c) //get value via ADC channel object
        {
            return adcChannels[c].getCurrentSample();
        }

        void releaseADC(int c) //release ADC for use by other threads
        {
            std::unique_lock <std::mutex> lockADC (mutexADC);

            std::string output = "ADC unlocked by thread " + std::to_string(c) + "\n";
            SetConsoleTextAttribute(hConsole, 7);
            cout << output;
            SetConsoleTextAttribute(hConsole, 7);

            lock.unlock();
            cond_var.notify_all();
        }

    private:
        Lock lock;
        std::vector<AdcInputChannel>& adcChannels; //vector reference
};


//4. Functions_________________________________________________________________________________________________
void run(ADC& adcChannels, int id, LinkAccessController& lac) //run function executed by each thread
{
    //4.1 Add ID of thread to thread map (with mutex)
    std::unique_lock<std::mutex> map_locker(mutexMAP);
    threadIDs.insert(std::make_pair(std::this_thread::get_id(), id)); //insert thread ID and id into the map
    map_locker.unlock();

    //4.2 Initialise the sampleBlock
    double sampleBlock[DATA_BLOCK_SIZE] = {0.0}; //initialises all values to 0

    //4.3 Acquire value from ADC & transmit to Receiver
    for (int i=0; i<DATA_BLOCK_SIZE; i++)
    {
        int threadID = search(); //write thread ID to an int for printing

        //4.3.1 Acquire ADCChannel
        adcChannels.requestADC(threadID);

        //4.3.2 Sample ADC via ADCChannel
        sampleBlock[i] = adcChannels.sampleADC(threadID);
        std::string output432 = "\t sample value from thread " + std::to_string(threadID) + " = " + std::to_string(sampleBlock[i]) + " \n";
        SetConsoleTextAttribute(hConsole, 7);
        cout << output432;
        SetConsoleTextAttribute(hConsole, 7);

        //4.3.3 Release ADC
        adcChannels.releaseADC(threadID);

        //4.3.4 Wait random time (100-500ms)
        int t1 = dis(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(t1));
        //std::string output434 = "Slept for: " + std::to_string(t1) + "ms. \n";
        //cout << output434;

        //4.3.5 Acquire link
        Link link = lac.requestLink();

        //4.3.6 Wait random time (100-500ms)
        int t2 = dis(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(t2));
        //std::string output436 = "Slept for: " + std::to_string(t2) + "ms. \n";
        //cout << output436;

        //4.3.7 Transmit data to Receiver via Link
        std::string output437 = "\t transmitting data to receiver from thread " + std::to_string(threadID) + " \n";
        SetConsoleTextAttribute(hConsole, 7);
        cout << output437;
        SetConsoleTextAttribute(hConsole, 7);
        link.writeToDataLink(threadID, sampleBlock);

        //4.3.8 Release link
        lac.releaseLink(link);
    }
}

int search() //a function allowing to print thread IDs cleanly
{
    std::map <std::thread::id, int>::iterator it = threadIDs.find(std::this_thread::get_id()); //build the interator

    if(it == threadIDs.end())
    {
        return -1; //thread 'id' NOT found
    }
    else
    {
        return it->second; //thread 'id' found, returns the associated integer
    }
}


//5. Main______________________________________________________________________________________________________
int main()
{
    //5.1 Initialise the ADC channels, receiver and LinkAccessController
    std::vector<AdcInputChannel> adcChannels; //Initialise the ADC channels

    for(int i = 0; i < MAX_NUM_OF_CHAN; i++)
    {
        adcChannels.push_back(i);
    }

    ADC adcObj(adcChannels); //instantiates the ADC
    Receiver rcv; //instantiates the receiver
    LinkAccessController lac(rcv); //instantiates the LinkAccessController

    //5.2 Create, instantiate and start the threads
    std::thread the_threads[MAX_NUM_OF_THREADS]; //create array of threads

    for (int i = 0; i < MAX_NUM_OF_THREADS; i++) //instantiate threads
    {
        the_threads[i] = std::thread(run, std::ref(adcObj), i, std::ref(lac));
    }

    for (int k = 0; k < MAX_NUM_OF_THREADS; k++) //attach the threads
    {
        the_threads[k].join();
    }

    //5.3 Print the 2D data array
    cout << endl;
    rcv.printBlocks();

    //5.4 Wait for the threads to finish
    SetConsoleTextAttribute(hConsole, 10);
    cout << endl << endl << "All threads terminated" << endl;
    SetConsoleTextAttribute(hConsole, 7);

    //5.5 End main
    return 0;
}

//6. Debug status______________________________________________________________________________________________
//debugged
