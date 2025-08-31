#include <asm-generic/socket.h>
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <math.h>
#include <string>
#include <unordered_map>


int min(int a , int b){
    return (a > b ? b : a);
}

std::string getContentType(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html"},
        {".htm",  "text/html"},
        {".css",  "text/css"},
        {".js",   "application/javascript"},
        {".json", "application/json"},
        {".png",  "image/png"},
        {".jpg",  "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif",  "image/gif"},
        {".svg",  "image/svg+xml"},
        {".ico",  "image/x-icon"},
        {".txt",  "text/plain"},
        {".pdf",  "application/pdf"},
        {".mp4",  "video/mp4"},
        {".webm", "video/webm"},
        {".ogg",  "audio/ogg"},
        {".mp3",  "audio/mpeg"},
        {".wav",  "audio/wav"}
    };

    // find last dot
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos);
        auto it = mimeTypes.find(ext);
        if (it != mimeTypes.end()) {
            return it->second;
        }
    }
    // fallback if unknown
    return "application/octet-stream";
}



void send_file(std::string file_path , int clientSocket){
    if(file_path == "data/")file_path = "data/index.html";
    std::ifstream infile(file_path , std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "invalid file path";
        return ;
    }

    infile.seekg(0 , std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0 , std::ios::beg);

    std::string http_header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + getContentType(file_path)+"\r\n"
        "Content-Length: " + std::to_string(file_size) + "\r\n"
        "Connection: close"
        "\r\n\r\n";

    send(clientSocket , (void*)http_header.c_str() , (http_header).size() , 0);

    int chunk_size = 8192;

    int numChunks = (file_size + chunk_size - 1)/chunk_size;


    for(int i = 0 ; i < numChunks ; i ++){
        char buffer[chunk_size];
        size_t bytesToRead = min(chunk_size, file_size - (i * chunk_size));
        infile.read(buffer, bytesToRead);
        send(clientSocket , (void *)buffer , bytesToRead , 0);
    }
    infile.close();
}


int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket failed");
        std::cout << strerror(errno);
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8000);

    int opt  = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR , (void *)&opt , sizeof(opt));
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        std::cout << strerror(errno);
        return 1;
    }

    if (listen(serverSocket, 2) < 0) {
        perror("Listen failed");
        std::cout << strerror(errno);
        return 1;
    }
    std::cout << "Server listening on port 8000...\n";


    while(true){
        clientSocket = accept(serverSocket, (struct sockaddr*)&address, &addrlen);
        if (clientSocket < 0) {
            perror("Accept failed");
            std::cout << strerror(errno);
            return 1;
        }
        std::cout << "Client connected!\n";
        char request[1024] = {0};
        const ssize_t bytesRecived = recv(clientSocket, request, sizeof(request), 0);
        std::string message(request , bytesRecived);
        std::istringstream reqStream(message);
        std::string method , file_path , version;
        reqStream >> method >> file_path >> version;
        std::cout << file_path;
        send_file("data" + file_path, clientSocket);
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}

