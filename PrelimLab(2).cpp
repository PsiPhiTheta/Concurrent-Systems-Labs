//Author: Thomas Hollis (9563426)
//Project: Concurrent Systems labs (University of Manchester)
//Year: 2017

//0.1 Inclusions (headers)
#include <iostream>
#include <thread>
#include <mutex>

//0.2 Inclusions (namespaces)
using namespace std;

//0.3 Definitions (macros)
#define PSLEEP 100
#define CSLEEP 200

//1. Main variables
std::mutex m;

//2. Classes
class Buffer
{
    public:
        Buffer(): count(0) {}
        void put()
        {
            if (count < 10)
            {
                count++;
                cout << "Producer thread" << std::this_thread::get_id() << ", count = " << count << endl;
            }
            else if (count == 10)
            {
                cout << "Buffer is full, producer thread is about to suspend" << endl;
            }
            else
            {
                cout << "Error: buffer overflow";
            }
        }
        int get()
        {
            if (count > 0)
            {
                count--;
                cout << "Producer thread" << std::this_thread::get_id() << ", count = " << count << endl;
            }
            else if (count == 0)
            {
                cout << "Buffer is empty, consumer thread is about to suspend" << endl;
            }
            else
            {
                cout << "Error: buffer overflow";
            }
            return 1;
        }
    private:
        int count;
};

class TClass
{
    public:
        TClass(Buffer &b): buffer1(b) {}
        void prods()
        {
            for (int i=0; i<100 ; i++)
            {
                m.lock();
                buffer1.put();
                m.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(PSLEEP));
            }
        }
        void cons()
        {
            for (int i=0; i<100 ; i++)
            {
                m.lock();
                buffer1.get();
                m.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(CSLEEP));
            }

        }
    private:
        Buffer &buffer1;
};



//3. Global functions


//4. Main function
int main()
{
	Buffer bufferMain;
	TClass tClassMain(bufferMain);
    std::thread t[5];
	t[0] = std::thread(&TClass::prods, &tClassMain); //might need to be replaced with ref(tc)
	t[1] = std::thread(&TClass::prods, &tClassMain); //might need to be replaced with ref(tc)
	t[2] = std::thread(&TClass::prods, &tClassMain); //might need to be replaced with ref(tc)
	t[3] = std::thread(&TClass::cons, &tClassMain); //might need to be replaced with ref(tc)
	t[4] = std::thread(&TClass::cons, &tClassMain); //might need to be replaced with ref(tc)
	for (int i=0; i<5; i++)
    {
        t[i].join();
    }
    cout << "All threads terminated." << endl;
}
