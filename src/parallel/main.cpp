#include <iostream>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unordered_map>
#include <cstring>
#include <sstream>

using namespace std;

// Constants
const int num_threads = 10;

// Global Variables
queue<int> clients;
unordered_map<string, string> KV_DATASTORE;
pthread_mutex_t map_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

// Function Declarations
void handleConnection(int client_fd);
void* startRoutine(void *);
int getServerSocket(const int &port);
void addToQueue(int client_fd);

int main(int argc, char **argv) {
    // Command line arguments check
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        exit(1);
    }

    // Port from command line argument
    int port = atoi(argv[1]);

    // Create server socket
    int server_fd = getServerSocket(port);
    if (server_fd < 0) {
        cerr << "Error: Failed to start server" << endl;
        exit(1);
    }

    // Listen on socket
    if (listen(server_fd, 5) < 0) {
        cerr << "Error: Couldn't listen on socket" << endl;
        close(server_fd);
        return -1;
    }

    cout << "Server listening on port: " << port << endl;

    // Variables for client
    sockaddr_in client_addr;
    socklen_t caddr_len = sizeof(client_addr);
    vector<pthread_t> thread_ids(num_threads);

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&thread_ids[i], NULL, &startRoutine, NULL);
    }

    // Accept connections
    while (true) {
        int client_fd = accept(server_fd, (sockaddr *)(&client_addr), &caddr_len);
        if (client_fd < 0) {
            cerr << "Error: Couldn't accept connection" << endl;
            exit(1);
        }
        addToQueue(client_fd);
    }

    // Destroy mutex locks
    pthread_mutex_destroy(&map_lock);
    pthread_mutex_destroy(&queue_lock);

    // Close socket
    close(server_fd);

    return 0;
}

int getServerSocket(const int &port) {
    // Create TCP socket and bind
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        cerr << "Error: Couldn't open socket" << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    socklen_t saddr_len = sizeof(server_addr);

    memset(&server_addr, 0, saddr_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, saddr_len) < 0) {
        cerr << "Error: Couldn't bind socket" << endl;
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void addToQueue(int client_fd) {
    pthread_mutex_lock(&queue_lock);
    clients.push(client_fd);
    pthread_mutex_unlock(&queue_lock);
    pthread_cond_signal(&queue_not_empty);
}

void* startRoutine(void *) {
    pthread_detach(pthread_self());

    while (true) {
        int client_fd = -1;
        pthread_mutex_lock(&queue_lock);
        while (clients.empty()) {
            pthread_cond_wait(&queue_not_empty, &queue_lock);
        }
        client_fd = clients.front();
        clients.pop();
        pthread_mutex_unlock(&queue_lock);

        if (client_fd != -1) {
            handleConnection(client_fd);
        }
    }
    pthread_exit(NULL);
}

void handleConnection(int client_fd) {
    char buffer[1024];
    bool end = false;
    string response, key, value;

    while (!end) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytesReceived < 0) {
            cerr << "Error: Couldn't receive message" << endl;
            exit(1);
        }
        else if (bytesReceived == 0) {
            cout << "Client disconnected." << endl;
            break;
        }
        else {
            string query;
            stringstream strm(buffer);
            while (getline(strm, query)) {
                if (query == "READ") {
                    getline(strm, key);
                    pthread_mutex_lock(&map_lock);
                    if (KV_DATASTORE.find(key) != KV_DATASTORE.end()) {
                        response = KV_DATASTORE[key] + "\n";
                    }
                    else {
                        response = "NULL\n";
                    }
                    pthread_mutex_unlock(&map_lock);
                }
                else if (query == "WRITE") {
                    getline(strm, key);
                    getline(strm, value);
                    value = value.substr(1);
                    pthread_mutex_lock(&map_lock);
                    KV_DATASTORE[key] = value;
                    response = "FIN\n";
                    pthread_mutex_unlock(&map_lock);
                }
                else if (query == "COUNT") {
                    pthread_mutex_lock(&map_lock);
                    response = to_string(KV_DATASTORE.size()) + "\n";
                    pthread_mutex_unlock(&map_lock);
                }
                else if (query == "DELETE") {
                    getline(strm, key);
                    pthread_mutex_lock(&map_lock);
                    if (KV_DATASTORE.find(key) != KV_DATASTORE.end()) {
                        KV_DATASTORE.erase(key);
                        response = "FIN\n";
                    }
                    else {
                        response = "NULL\n";
                    }
                    pthread_mutex_unlock(&map_lock);
                }
                else if (query == "END") {
                    end = true;
                    break;
                }
                send(client_fd, response.c_str(), response.length(), 0);
                response.clear();
                key.clear();
                value.clear();
            }
        }
    }
    close(client_fd);
}
