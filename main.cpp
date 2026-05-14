/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: apaz-mar <apaz-mar@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/07 22:03:27 by apaz-mar          #+#    #+#             */
/*   Updated: 2026/05/14 11:32:58 by apaz-mar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>     // for socket()
#include <sys/types.h>      // for socket()
#include <netinet/in.h>     // for sockaddr_in, AF_INET, INADDR_ANY, htons, ntohns
#include <arpa/inet.h>      // inet_pton, inet_ntop
#include <netdb.h>          // getnamenfo, NI_MAXHOST, NI_MAXSERV

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./server <port>\n";
        return (1);
    }
    
    int port = std::atoi(argv[1]);

    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);        // AF_INET is the format of IPv4, alternatively it would be AF_INET6 for IPv6
    if (server_fd < 0)
    {
        std::cerr << "socket failed\n";
        return (1);
    }

    // 2. Allow reusing the port quickly after restart

    // 3. Bind socket to IP / Port... Port number could be 54000 to play with it
    sockaddr_in hint;             // in is for IPv4
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);    

    //internet command, pointer to a string to a number.....  so from "127.0.0.1" to .... 
    //second argument is any address as string; third argument is the buffer, the buffer is a pointer
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);     

    // The actual binding. Second argument is the address of the socket; third is the length
    if (bind(server_fd, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        std::cerr << "can't bind to IP/port\n";
        return (1);    
    }

    // 4. Mark the socket for being able to listen. SOMAXCONN is the maximum number of connections, around 128
    if (listen(server_fd, SOMAXCONN) == -1)
    {
        std::cerr << "can't listen\n";
        return (1);    
    }

    // 5. Accept a call
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];

    // we want to accept an incoming connection
    int clientSocket = accept(server_fd, (sockaddr*)&client, &clientSize);
    if (clientSocket == -1)
    {
        std::cerr << "Problem with client connecting\n";
        return (1);    
    }

    // 6. close the listening socket
    close(server_fd);

    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    // who is connected to me?
    int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);
    if (result == 0)    //getnameinfo returns 0 on success
    {
        std::cout << host << " connected on " << svc << std::endl;
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
    }

    // 7. While receiving, display message, echo message
    char buf[4096];
    while (true)
    {
        //clear the buffer
        memset(buf, 0, 4096);
        //wait for a message
        int bytesRecv = recv(clientSocket, buf, 4096, 0);
        if (bytesRecv == -1)
        {
            std::cerr << "There was a connection issue" << std::endl;
            break ;
        }
        if (bytesRecv == 0)
        {
            std::cout << "The client disconnected" << std::endl;
            break ;
        }

        //Display message
        std::cout << "Received: " << std::string(buf, bytesRecv) << std::endl;
        //Resend message
        send(clientSocket, buf, bytesRecv, 0);
    }

    //8. Close socket
    close(clientSocket);

    return (0);
}