#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <bits/stdc++.h>
#include <fstream>
#include <mutex>

using namespace std;
mutex mtx[100];

mutex count_mtx;
int clints = 0;

#include "../helper.cpp"
#define OK_RESPONSE "HTTP/1.1 200 OK\r\n\r\n"
#define FILE_NOT_FOUND_RESPONSE "HTTP/1.1 404 Not Found\r\n\r\n"

int sock_fd;


void parseRequest(const std::string& request, std::string& hostname, int& port, std::string& path) {
    std::istringstream iss(request);
    iss >> path >> hostname >> port;
}


int countWords(const std::string& str) {
    std::istringstream iss(str);
    int count = 0;
    std::string word;

    while (iss >> word) {
        count++;
    }

    return count;
}

// Modified handleGet function
void handleGetWebServer(int sock, const string& request) {
    std::cout << request.substr(0, 50) << std::endl;

    std::string path, hostname;
    int port;
    parseRequest(request, hostname, port, path);

    // Create socket for communication with the real web server
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "Cannot open socket" << std::endl;
        return;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert hostname to IP address
    if (inet_pton(AF_INET, hostname.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        return;
    }

    // Connect to the real server
    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return;
    }

    // Send the request to the real server
    std::string forward_request = "GET " + path + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    send(server_sock, forward_request.c_str(), forward_request.size(), 0);

    // Read the response from the real server
    char buffer[4096] = {0};
    int bytes_read = read(server_sock, buffer, 4096);

    // Forward the response to the client
    mtx[sock].lock();
    send(sock, buffer, bytes_read, 0);
    mtx[sock].unlock();

    // Close the server socket
    close(server_sock);
}

void handleGet(int sock, string request)
{
    cout << request.substr(0, 50) << endl;

    if(countWords(request)==4){
        handleGetWebServer(sock,request);
        return;
    }
    string fileName = getFileName(request);
    if (!fileExist(fileName))
    {
        mtx[sock].lock();
        commuincate().sendMessage(NOT_FOUND_RES, sock);
        mtx[sock].unlock();
        return;
    }
    else
    {
        if (isImage(fileName))
        {
            mtx[sock].lock();
            commuincate().sendImage(fileName, sock, OK);
            mtx[sock].unlock();
        }
        else
        {
            string fileData = getDataFromFile(fileName);
            string header = addHeader(fileData.size());
            mtx[sock].lock();
            commuincate().sendMessage(OK + header + fileData, sock);
            mtx[sock].unlock();
        }
    }
}
void handlePost(int sock, string request)
{
    string p;
    int i = 0;
    while (request[i] != '\n')
    {
        p += request[i];
        i++;
    }
    p += request[i];
    cout << p << endl;
    string fileName = getFileName(request);
    if (isImage(fileName))
    {
        string image = getDataFromString(request);
        saveImage(image, fileName);
        mtx[sock].lock();
        commuincate().sendMessage(OK, sock);
        mtx[sock].unlock();
    }
    else
    {
        string dataFromRes = getDataFromString(request);
        saveFile(dataFromRes, fileName);
        mtx[sock].lock();
        commuincate().sendMessage(OK, sock);
        mtx[sock].unlock();
    }
    return;
}
void *handle(void *socket_desc)
{

    count_mtx.lock();
    clints++;
    double time = 1.0 * DEFUALT_TIME / clints;
    if (time < MIN_TIME)
        time = MIN_TIME;
    count_mtx.unlock();
    int sock = *(int *)socket_desc;

    while (1)
    {
        //mtx[sock].lock();
        count_mtx.lock();
        string mes = commuincate().reciveMessage(sock, time, true);
        count_mtx.unlock();
        //mtx[sock].unlock();
        if (mes.size() == 0)
        {
            count_mtx.lock();
            clints--;
            count_mtx.unlock();
            return 0;
        }
        bool isPost = isPostRequest(mes);
        if (isPost)
        {
            handlePost(sock, mes);
        }
        else
        {
            handleGet(sock, mes);
        }
    }
    count_mtx.lock();
    clints--;
    count_mtx.unlock();
}

int main(int argc, char const *argv[])
{
    int new_sock, c;
    struct sockaddr_in server, client;
    int *temp;
    int port = stringToInt(argv[1]);
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd == -1)
    {
        puts("error");
        return -1;
    }
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("error 2 ");
        return -1;
    }
    int l = listen(sock_fd, 10);
    if (l < 0)
    {
        puts("error while listinng ");
        exit(1);
    }

    c = sizeof(struct sockaddr_in);

    while ((new_sock = accept(sock_fd, (struct sockaddr *)&client, (socklen_t *)&c)) > 0)
    {
        temp = &new_sock;
        pthread_t proccess_conn;
        if (pthread_create(&proccess_conn, NULL, handle, (void *)temp) < 0)
        {
            return -1;
        }
        // pthread_join(proccess_conn, NULL);
    }

    return 0;
}
