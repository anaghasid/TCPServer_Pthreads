/* 
 * tcpserver.c - A multithreaded TCP echo server 
 * usage: tcpserver <port>
 * 
 * Testing : 
 * nc localhost <port> < input.txt
 */

// Source: https://github.com/bozkurthan/Simple-TCP-Server-Client-CPP-Example
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
#include <queue>
#include <map>
#include<vector>

using namespace std;

map<string, string> datastore;


// void* sendMessage(int clientSocket, string message, )
//to handle client connections
void* handleClient(int clientSocket) {
        char msg[1500];
        int bytesRead = 0;
        int bytesWritten = 0;
        // Receive message from client
        memset(&msg, 0, sizeof(msg));
        bytesRead = recv(clientSocket, msg, sizeof(msg), 0);
        if (bytesRead <= 0) {
            // Error or connection closed by client
            return;
        }
        string request(msg);
        cout << "Client: " << msg << endl;
        
        int start=0, end = 0;
        vector<string> tokens;
        while ((end = request.find("\n", start)) != std::string::npos) {
              std::string token = request.substr(start, end - start);
              tokens.push_back(token);
              start = end + 1;
          }
          
        // Handle client communication
     for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "DELETE") {
            string key = tokens[i+1];
            i++;
            int dels = datastore.erase(key);
            if(dels<=0) {
                bytesWritten = send(clientSocket, "NULL\n", 5, 0);
                if (bytesWritten <= 0) {
                    break;
                }
            }
            else {
                bytesWritten = send(clientSocket, "FIN\n", 4, 0);
                if (bytesWritten <= 0) {
                    break;
                }
            }
        } 
        
        else if (tokens[i] == "WRITE") {
            string key = tokens[i+1];
            string value = tokens[i+2];
            value = value.substr(1);
            i = i+2;
            datastore[key] = value;
            cout<<"Writing "<<value<<" to "<<key<<endl;
            bytesWritten = send(clientSocket, "FIN\n", 4, 0);
            if (bytesWritten <= 0) {
                break;
            }
        } 

        if (tokens[i] == "COUNT") {
            string count = to_string(datastore.size());
            cout<<"Count is "<<count;
            count.append("\n");
            bytesWritten = send(clientSocket, count.c_str(), count.size(), 0);
            if (bytesWritten <= 0) {
                break;
            }
        } 
        
        else if (tokens[i] == "READ") {
            string key = tokens[i+1];
            cout<<"Inside read key = "<<key<<endl;
            i++;
            string value;
            if(datastore.count(key)) {
                value = datastore[key];
                value.append("\n");
            }
            if(datastore.count(key)) {
                bytesWritten = send(clientSocket, value.c_str(), value.size(), 0);
                if (bytesWritten <= 0) {
                break;
                }
            }
            else {
                bytesWritten = send(clientSocket, "NULL\n", 5, 0);
                if (bytesWritten <= 0) {
                break;
            }
            }
        }

        else if (tokens[i] == "END") {
            close(clientSocket);
            cout<<"Closing client\n"<<endl;
            break;
        }
        
        else {
            break;
        }
     }
}



int main(int argc, char *argv[]) {
    //for the server, we only need to specify a port number
    if(argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    //grab the port number
    int port = atoi(argv[1]);
    //buffer to send and receive messages with
    char msg[1500];
     
    //setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSd < 0) {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }
    //bind the socket to its local address
    int bindStatus = bind(serverSd, (struct sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    cout << "Waiting for a client to connect..." << endl;
    //listen for up to 5 requests at a time
    listen(serverSd, 5);
    //receive a request from client using accept
    //we need a new address to connect with the client
    
    while (true) {
        // Accept a new client connection
        sockaddr_in newSockAddr;
        socklen_t newSockAddrSize = sizeof(newSockAddr);
        int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if (newSd < 0) {
            cerr << "Error accepting request from client!" << endl;
            continue;
        }

        // Add the new client socket to the queue
        // clientQueue.push(newSd);
        handleClient(newSd);
    }
    return 0;
}