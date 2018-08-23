//Name: Martin Joshy
//PSID: 1495688
//Class: Principles of Operating Systems (COSC 3360)
//Homework 3 - The Whittier Tunnel

#ifdef __unix__
#include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * x)
#endif

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <string>
#include <signal.h>
#include <unistd.h>
using namespace std;


static int numCarsAllowedInTunnel; //Maximum number of cars in the tunnel
string tunnelStatus; //status of tunnel. 1 is WB, 2 is BB, 0 is blocked
static int currentNumCarsInTunnel; // Current number of cars in the tunnel
static int carsCrossedBV; //Number of Bear Valley-bound cars that crossed the tunnel
static int carsCrossedWB; //Number of Withier-bound cars that crossed the tunnel
static int numTotalCarsWait; //Number of cars that had to wait because there were too many car in the tunnel

static pthread_mutex_t mylock;//mutex for changing current cars in tunnel

//conditional to signal changes for the tunnel direction and number of cars in tunnel
pthread_cond_t clear = PTHREAD_COND_INITIALIZER;


struct Car {
public:
	int carDelayTime, timeToCrossTunnel, carNumber;
	string direction;

	Car(int a, string b, int c, int d) {
		carDelayTime = a;
		direction = b;
		timeToCrossTunnel = c;
		carNumber = d;
	}
};

void *tunnelProcess(void *arg) {
	void terminationSignal(int sig);
	int done = 0;

	while (true){
		cout << "The tunnel is now open to Whittier-bound traffic. \n";
		tunnelStatus = "WB";
		pthread_cond_broadcast(&clear);
		sleep(5);

		cout << "The tunnel is now closed to ALL traffic. \n";
		tunnelStatus = "CLOSED";
		pthread_cond_broadcast(&clear);
		sleep(5);

		cout << "The tunnel is now open to Bear Valley-bound traffic. \n";
		tunnelStatus = "BB";
		pthread_cond_broadcast(&clear);
		sleep(5);

		cout << "The tunnel is now closed to ALL traffic. \n";
		tunnelStatus = "CLOSED";
		pthread_cond_broadcast(&clear);
		sleep(5);
	} 
}

void *carProcess(void * arg) {
	Car *thisCar;
	thisCar = (Car *) arg;

	pthread_mutex_lock(&mylock);
	cout << "Car #" << thisCar->carNumber << " going to " << thisCar->direction << " has arrived at tunnel " << endl;
	pthread_mutex_unlock(&mylock);

	pthread_mutex_lock(&mylock);
	while (true) {
		//if the tunnel is going in the appropriate direction and has room for this car it goes through
		if (tunnelStatus == thisCar->direction && currentNumCarsInTunnel < numCarsAllowedInTunnel) {
			if (thisCar->direction == "WB") {
				cout << "Car # " << thisCar->carNumber << " going to Whittier enters the tunnel.\n";
				carsCrossedWB++;
			}
			else {
				cout << "Car # " << thisCar->carNumber << " going to Bear Valley enters the tunnel.\n";
				carsCrossedBV++;
			}
			currentNumCarsInTunnel++;
			pthread_mutex_unlock(&mylock);
			sleep(thisCar->timeToCrossTunnel);
			break;
		}
		//if the tunnel is going in the appropriate direction but there is no room it waits for there to be room in the tunnel
		else if (currentNumCarsInTunnel == numCarsAllowedInTunnel && thisCar->direction == tunnelStatus){
			numTotalCarsWait++;
		}
		//if the tunnel is not going in the appropriate direction it is waiting for the tunnel thread to broadcast the change in direction
		pthread_cond_wait(&clear, &mylock);
	}

	pthread_mutex_lock(&mylock);
	currentNumCarsInTunnel--;
	cout << "Car # " << thisCar->carNumber << " going to " << thisCar->direction << " exits the tunnel" << endl;
	pthread_cond_broadcast(&clear);
	pthread_mutex_unlock(&mylock);
	pthread_exit(NULL);

}

int main() {
	string first, second , third;
	cin >> first;
	numCarsAllowedInTunnel = stoi(first);

	int carCount = 0;
	int errCar, errTunnel;
	int sig = 0;

	pthread_t carThreads[128];
	
	//creating tunnel thread
	pthread_t tunnelThread;
	errTunnel = pthread_create(&tunnelThread, NULL, tunnelProcess, (void *) &first);

	//creating all car threads
	while (cin >> first && cin >> second && cin >> third) {
		Car *temp = new Car(stoi(first), second, stoi(third), carCount + 1);
		sleep(temp->carDelayTime);
		errCar = pthread_create(&carThreads[carCount], NULL, carProcess, (void *)temp);
		carCount++;
	}

	for (int i = 0; i < carCount; i++) {
		pthread_join(carThreads[i], NULL);
	}

	cout << carsCrossedBV << " car(s) going to Bear Valley arrived at the tunnel.\n";
	cout << carsCrossedWB << " car(s) going to Whittier arrived at the tunnel.\n";
	cout << numTotalCarsWait << " car(s) had to wait because the tunnel was full.\n";

	cout << endl;

	pthread_kill(tunnelThread,1);
	pthread_exit(NULL);
	return 0;
}