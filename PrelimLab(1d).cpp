//Author: Thomas Hollis (9563426)
//Project: Concurrent Systems labs (University of Manchester)
//Year: 2017

//0.1 Inclusions (headers)
#include <iostream>
#include <thread>
#include <mutex>

//0.2 Inclusions (namespaces)
using namespace std;

//1. Main variables
std::mutex m;

//2. Classes
class TClass
{
    public:
        TClass(int x): y(x) {}
        void run()
        {
            m.lock();
            cout << "Hello, I am thread " << std::this_thread::get_id() << ", y = " << y << endl;
            y++;
            m.unlock();
        }
    private:
        int y;

};

//3. Global functions


//4. Main function
int main()
{
	TClass tc(1);
	std::thread t[5];
	for (int i=0; i<5; i++)
    {
        t[i] = std::thread(&TClass::run, std::ref(tc));
    }
	for (int i=0; i<5; i++)
    {
        t[i].join();
    }
    cout << "All threads terminated." << endl;
}
