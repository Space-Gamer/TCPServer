#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>

using namespace std;

int processClientRequest(int &);
int createServerSocket(const int &);
unordered_map<string, string> DataStore;

int main(int argc, char **argv)
{
    int serverPort;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serverPort = atoi(argv[1]);
    int serverSocket = createServerSocket(serverPort);

    if (listen(serverSocket, 5) < 0)
    {
        cerr << "Error: Unable to listen on socket" << endl;
        close(serverSocket);
        return -1;
    }

    cout << "Server listening on port: " << serverPort << endl;

    sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    vector<int> clientSockets;

    while (true)
    {
        int clientSocket = accept(serverSocket, (sockaddr *)(&clientAddress), &clientAddressLength);
        if (clientSocket < 0)
        {
            cerr << "Error: Unable to accept connection" << endl;
            exit(1);
        }

        int res = processClientRequest(clientSocket);

        if (res < 0)
        {
            cerr << "Error: Unable to process client request" << endl;
            exit(1);

        }
    }

    close(serverSocket);
    return 0;
}

int createServerSocket(const int &port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        cerr << "Error: Unable to open socket" << endl;
        return -1;
    }

    struct sockaddr_in serverAddress;
    socklen_t serverAddressLength = sizeof(serverAddress);

    memset(&serverAddress, 0, serverAddressLength);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, serverAddressLength) < 0)
    {
        cerr << "Error: Unable to bind socket" << endl;
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

int processClientRequest(int &clientSocket)
{
    char clientBuffer[1024];
    bool connectionEnd = false;
    string serverResponse;
    string dataKey, dataValue;

    while (!connectionEnd)
    {
        memset(clientBuffer, 0, sizeof(clientBuffer));
        int receivedBytes = recv(clientSocket, clientBuffer, sizeof(clientBuffer), 0);
        if (receivedBytes < 0)
        {
            cerr << "Error: Unable to receive message" << endl;
            exit(1);
        }
        else if (receivedBytes == 0)
        {
            cout << "Client disconnected." << endl;
            break;
        }
        else
        {
            string clientQuery;
            stringstream clientStream(clientBuffer);
            while (getline(clientStream, clientQuery))
            {
                if (clientQuery == "READ")
                {
                    getline(clientStream, dataKey);
                    if (DataStore.find(dataKey) != DataStore.end())
                    {
                        serverResponse = DataStore[dataKey] + "\n";
                    }
                    else
                    {
                        serverResponse = "NULL\n";
                    }
                }
                else if (clientQuery == "WRITE")
                {
                    getline(clientStream, dataKey);
                    getline(clientStream, dataValue);
                    dataValue = dataValue.substr(1);
                    DataStore[dataKey] = dataValue;
                    serverResponse = "FIN\n";
                }
                else if (clientQuery == "COUNT")
                {
                    serverResponse = to_string(DataStore.size()) + "\n";
                }
                else if (clientQuery == "DELETE")
                {
                    getline(clientStream, dataKey);
                    if (DataStore.find(dataKey) != DataStore.end())
                    {
                        DataStore.erase(dataKey);
                        serverResponse = "FIN\n";
                    }
                    else
                    {
                        serverResponse = "NULL\n";
                    }
                }
                else if (clientQuery == "END")
                {
                    connectionEnd = true;
                    break;
                }
                send(clientSocket, serverResponse.c_str(), serverResponse.length(), 0);
                serverResponse.clear();
                dataKey.clear();
                dataValue.clear();
            }
        }
    }

    int result = close(clientSocket);
    return result;
}