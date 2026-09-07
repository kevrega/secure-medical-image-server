#ifndef TCP_SERVER_H
#define TCP_SERVER_H

class TcpServer
{
public:
    TcpServer(int port);
    ~TcpServer();

    void start();

private:
    int port; // Server listens from port
    int server_socket; // Communication endpoint
};

#endif