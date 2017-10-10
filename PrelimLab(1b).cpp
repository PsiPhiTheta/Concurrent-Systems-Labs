#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

std::mutex m;

void run(int id)
{
    m.lock();
    for (int i = 0; i < 5; i++)
    {
        cout << "Hello, I'm thread " << id << endl;
    }
    m.unlock();
}

int main()
{
	std::thread t[5];

	for (int i=0; i<5; i++)
    {
        t[i] = std::thread(run, i);
    }

	for (int i=0; i<5; i++)
    {
        t[i].join();
    }

    cout << "All threads terminated." << endl;
}
