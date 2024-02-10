#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <pthread.h>
#include <queue>
#include <map>

using namespace std;

// Mutexes 
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;

// Queue to hold incoming client connections
queue<int> clientQueue;

// Datastore as a map<string, string>
map<string, string> datastore;


// void* sendMessage(int clientSocket, string message, )
//to handle client connections
void* handleClient(void* arg) {
    while (true) {
        int clientSocket;

        // Lock the queue mutex before accessing the queue
        pthread_mutex_lock(&queueMutex);
        if (!clientQueue.empty()) {
            // Get the client socket from the queue
            clientSocket = clientQueue.front();
            clientQueue.pop();
        } else {
            // Release the mutex and sleep for a while if the queue is empty
            pthread_mutex_unlock(&queueMutex);
            usleep(10000); // Sleep for 10 milliseconds
            continue;
        }
        // Release the queue mutex after accessing the queue
        pthread_mutex_unlock(&queueMutex);

        char msg[1500];
        int bytesRead = 0;
        int bytesWritten = 0;
        
        // Handle client communication
        // int pos = 0;
        while (true) {
            // Receive message from client
            memset(&msg, 0, sizeof(msg));
            bytesRead = recv(clientSocket, msg, sizeof(msg), 0);
            if (bytesRead <= 0) {
                // Error or connection closed by client
                break;
            }
            cout << "Client: " << msg << endl;


            string request(msg);
            int pos = 0;
            string command = request.substr(pos, request.find("\n"));
            pos += request.find("\n", pos);
            cout<<"Command is "<<command<<endl;

            if (command == "DELETE") {
              string key = request.substr(pos + 1,request.find("\n"));
              pos += request.find("\n", pos);
              cout<<"Inside delete key is "<<key<<endl;
              pthread_mutex_lock(&mapMutex);
              int dels = datastore.erase(key);
              pthread_mutex_unlock(&mapMutex);
              if(dels<=0) {
                bytesWritten = send(clientSocket, "FIN", 3, 0);
                if (bytesWritten <= 0) {
                  break;
                }
              }
            }

            if (command == "READ") {
              cout<<"Inside read"<<endl;
              string key = request.substr(pos + 1);
              pos += request.find("\n", pos);;
              string value;
              pthread_mutex_lock(&mapMutex);
              if(datastore.count(key))
                value = datastore[key];
                cout<<"Read value "<<value<<" for key "<<key<<endl;
              pthread_mutex_unlock(&mapMutex);
              if(datastore.count(key)) {
                bytesWritten = send(clientSocket, (char*)&value, value.size(), 0);
                if (bytesWritten <= 0) {
                  break;
                }
              }
              else {
                bytesWritten = send(clientSocket, "NULL", 4, 0);
                if (bytesWritten <= 0) {
                  break;
              }
              }
            } 

            if(command == "WRITE") {
              string key = request.substr(pos + 1);
              pos += request.find("\n", pos);
              string value = request.substr(pos + 1);
              pos += request.find("\n", pos);
              pthread_mutex_lock(&mapMutex);
              datastore[key] = value;
              cout<<"Writing "<<key<<" to "<<value<<endl;
              pthread_mutex_unlock(&mapMutex);
              bytesWritten = send(clientSocket, "FIN", 3, 0);
                if (bytesWritten <= 0) {
                  break;
                }
            }

            if(command=="COUNT") {
              string count = to_string(datastore.size());
              cout<<"Count is "<<count;
              bytesWritten = send(clientSocket, (char*)&count, count.size(), 0);
                if (bytesWritten <= 0) {
                  break;
                }
            }

            if(command=="END") {
              close(clientSocket);
              cout<<"Closing client"<<endl;
              break;
            }

            // Example processing: Echo back to the client
            // bytesWritten = send(clientSocket, msg, strlen(msg), 0);
            // if (bytesWritten <= 0) {
            //     // Error writing to client
            //     break;
            // }
        }

        // Close client socket
        // close(clientSocket);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // Check for correct usage
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // Grab the port number
    int port = atoi(argv[1]);

    // Setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    // Open stream oriented socket with internet address
    // Also keep track of the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSd < 0) {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }

    // Bind the socket to its local address
    int bindStatus = bind(serverSd, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if (bindStatus < 0) {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }

    cout << "Waiting for clients to connect..." << endl;

    // Listen for incoming connections
    listen(serverSd, 5);

    // Create threads to handle clients
    const int NUM_THREADS = 5; // Number of threads to handle clients
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, handleClient, NULL);
    }

    // Accept and handle incoming client connections
    while (true) {
        // Accept a new client connection
        sockaddr_in newSockAddr;
        socklen_t newSockAddrSize = sizeof(newSockAddr);
        int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if (newSd < 0) {
            cerr << "Error accepting request from client!" << endl;
            continue;
        }

        // Lock the queue mutex before accessing the queue
        pthread_mutex_lock(&queueMutex);
        // Add the new client socket to the queue
        clientQueue.push(newSd);
        // Release the queue mutex after accessing the queue
        pthread_mutex_unlock(&queueMutex);
    }

    // Close the server socket
    close(serverSd);
    return 0;
}
