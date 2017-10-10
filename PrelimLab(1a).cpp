#include <iostream>
#include <thread>

using namespace std;

void run(int id)
{
    for (int i = 0; i < 5; i++)
    {
        cout << "Hello, I'm thread " << id << endl;
    }
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
