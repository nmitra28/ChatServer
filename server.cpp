#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <string>
#include <regex>

using std::cout;
using std::endl;
using std::regex;
using std::regex_match;
using std::string;
using std::to_string;
using std::vector;

//********************************************************************************************
//********************************** Structs for the code ************************************ 
//********************************************************************************************

struct client;
struct room
{
    string id = "";
    string pw = "";
    vector<struct client> myClients;
};

struct client
{
    int fd = -1;
    string name = "";
    struct room myRoom;
};

vector<client> clients; // Tracks clients 
vector<room> rooms;     //track rooms

void disconnect(client cli);
void removeUser(client cli, room rm);
void addRoom(client cli, room rm);
void joinroom(client cli, room rm);
void leaveRoom(client cli);


//********************************************************************************************
//*********************** Helper methods to communicate with Client ************************** 
//********************************************************************************************

void reciever(int sock, uint8_t *buffer, size_t exp_b)
{
    size_t offset = 0, rec_b = 0;

    while (offset < exp_b)
    {
        rec_b = recv(sock, buffer + offset, exp_b - offset, 0);

        if (rec_b < 0)
        {
            perror("send() failed");
            exit(1);
        }

        offset += rec_b;
    }
}

void sender(int sock, uint8_t *buffer, size_t exp_b)
{

    size_t offset = 0, send_b = 0;

    while (offset < exp_b)
    {
        send_b = send(sock, buffer + offset, exp_b - offset, 0);

        if (send_b < 0)
        {
            perror("send() failed");
            exit(1);
        }

        offset += send_b;
    }
}



//********************************************************************************************
//****************************** Send appropriate commands *********************************** 
//********************************************************************************************




void join_message(int sock, int valid){

    uint8_t sendbuf[65536];
    string message = "Invalid password. You shall not pass.";

    if(valid >0) {
        sendbuf[0] = 0x00;
        sendbuf[1] = 0x00;
        sendbuf[2] = 0x00;
        sendbuf[3] = 0x01;
        sendbuf[4] = 0x04;
        sendbuf[5] = 0x17;
        sendbuf[6] = 0x9a;
        sendbuf[7] = 0x00;

        uint8_t* buffer = &sendbuf[0];

        sender(sock, buffer, 8);
    }
    else{
        *(uint32_t *)sendbuf = htonl(message.size() + 8);
        sendbuf[0] = 0x00;
        sendbuf[1] = 0x00;
        sendbuf[2] = 0x00;
        sendbuf[3] = 0x26;
        sendbuf[4] = 0x04;
        sendbuf[5] = 0x17;
        sendbuf[6] = 0x9a;
        sendbuf[8] = 0x01;

        uint8_t msg[message.size()];
        for (int i = 0; i < message.size(); i++)
        {
            msg[i] = (uint8_t)message.at(i);
        }
        memcpy(&sendbuf[8], &msg[0], message.size());
        sender(sock, sendbuf, message.size() + 8);
}

		uint8_t *ps = sendbuf;
		int i = 0;
		
}

void msg_ack(string name, client c, string message, uint8_t* bufferheader, int valid){

        uint8_t sendbuf[65536];

        *(uint32_t *)sendbuf = htonl(10 + name.size()  + message.size());

        memcpy(&sendbuf[0], bufferheader, 7);

        for (int i = 0; i < 7; i++)
        {
            sendbuf[i] = (uint8_t)bufferheader[i];
        }

        if(valid > 0){
            sendbuf[7] = 0x00;
            // (uint8_t)name.size();

            uint8_t nameInHex[name.size()];
            for (int i = 0; i < name.size(); i++)
            {
                nameInHex[i] = (uint8_t)name.at(i);
            }
            memcpy(&sendbuf[8], &nameInHex[0], name.size());
            sendbuf[8 + name.size() ] = 0x00;
            sendbuf[8 + name.size() + 1] = 0x00;
                // uint8_t)message.size();

            uint8_t msgInHex[name.size()];
            for (int i = 0; i < message.size(); i++)
            {
                msgInHex[i] = (uint8_t)message.at(i);
            }
            memcpy(&sendbuf[8 + name.size() + 2], &msgInHex[0], message.size());

            sender(c.fd, sendbuf, 10 + name.size()  + message.size());
        }

        else{
            sendbuf[3] = 0x11;
            sendbuf[6] = 0x9a;
            sendbuf[7] = 0x01;

            string negmsg = "Nick not present";
            uint8_t negHex[negmsg.size()];
            for (int i = 0; i < negmsg.size(); i++)
            {
                negHex[i] = (uint8_t)negmsg.at(i);
            }
            memcpy(&sendbuf[8], &negHex[0], negmsg.size());
            sender(c.fd, sendbuf, negmsg.size() + 8);
        }
      


        uint8_t *p = &sendbuf[0];
         int i =0; 
         printf("header ------- \n");
         while (i < 20)
             printf("%02x ", p[i++]);

        printf(" \n");

        


}


void default_ack(int clientSock, client cli, uint8_t const1, uint8_t const2)
{
    uint8_t sendbuf[65536];
    *(uint32_t *)sendbuf = htonl(cli.name.size() + 1);
    sendbuf[4] = const1;
    sendbuf[5] = const2;
    sendbuf[6] = 0x9a;
    sendbuf[7] = 0x00;
    uint8_t nameInHex[cli.name.size()];
    for (int i = 0; i < cli.name.size(); i++)
    {
        nameInHex[i] = (uint8_t)cli.name.at(i);
    }
    memcpy(&sendbuf[8], &nameInHex[0], cli.name.size());

    sender(clientSock, sendbuf, cli.name.size() + 8);

    uint8_t *ps = sendbuf;
    int i = 0;
}






//********************************************************************************************
//****************** Parsing and Processing recieved messages and commands ******************* 
//********************************************************************************************

string* myparser(uint8_t *buffermsg, client cli, int a)
{
    string* arr = new string[3];
    arr[0]="";
    arr[1]="";
    uint8_t size1 = buffermsg[0];
    uint8_t first[size1];

    if ((int)size1 > 0)
    {
        for (int i = 0; i < size1; i++)
        {
            first[i] = buffermsg[i + 1];
        }
        arr[0] = (char *)first;

        uint8_t size2 = buffermsg[size1 + 1 + a];
        uint8_t second[size2];

        if ((int)size2 > 0)
        {
            for (int i = 0; i <= size2; i++)
            {
                second[i] = buffermsg[size1 + 2 + i];
            }
            arr[1] = (char *)second;
        }
    }
    
    cout<<"first part  --- "<< arr[0]<<endl;
	
    cout<<"second part --- "<< arr[1]<<endl;

    return arr;
}


//********************************************************************************************
//************************** Functions to manage clients and message ************************* 
//********************************************************************************************

void disconnect(client cli)
{

    if (cli.myRoom.id.empty())
        ;
    else
    {
        leaveRoom(cli);
    }
    for (int j = 0; j < clients.size(); j++)
    {
        struct client curr = clients.at(j);
        if ((curr.name.compare(cli.name) == 0))
        {
            clients.erase(clients.begin() + j);
        }
    }
}

void addRoom(client cli, room rm)
{
    for (client i : clients)
    {
        if (i.name.compare(cli.name) == 0 & i.fd == cli.fd)
        {
            if (i.myRoom.id.empty())
            {
                i.myRoom = rm;
                rm.myClients.push_back(cli);
            }

            else
            {
                leaveRoom(i);
                i.myRoom = rm;

                rm.myClients.push_back(i);
            }
        }
    }
}
void leaveRoom(client cli)
{
    for (client i : clients)
    {
        if (i.name.compare(cli.name) == 0 & i.fd == cli.fd)
        {
            if (i.myRoom.id.empty())
            {
                disconnect(i);
            }
            else
            {

                removeUser(i, i.myRoom);
                i.myRoom.id = "";
                i.myRoom.pw = "";
                vector<client> myClients;
                i.myRoom.myClients = myClients;
            }
        }
    }
}

void removeUser(client cli, room rm)
{
    int pos = 0;
    for (room i : rooms)
    {
        if ((rm.id.compare(i.id) == 0) & (rm.pw.compare(i.pw)) == 0)
        {
            pos = 0;
            for (int j = 0; j < i.myClients.size(); j++)
            {
                struct client curr = i.myClients.at(j);
                if ((curr.name.compare(cli.name) == 0))
                {
                    i.myClients.erase(i.myClients.begin() + j);
                }
            }
        }
    }
}

void nickName(client cli, string name)
{
    regex reg("^rand[0-9]+$");
    int flag = 0;
    if (regex_match(name, reg))
    {
        perror("Nickname not valid format");
    }
    else
    {
        for (client i : clients)
        {

            if (regex_match(cli.name, reg) & i.name.compare(cli.name) == 0)
            {
                i.fd = -1;
                leaveRoom(i);
                cli.name = name;
                clients.push_back(cli);
                flag = 1;
            }
            else if (i.name.compare(cli.name) == 0 & i.fd == cli.fd)
            {
                i.name = name;
                flag = 1;
            }
        }

        if (flag == 0)
        {
            perror("client not found");
        }
    }
}


void joinroom(uint8_t *buffermsg, client cli)
{
        string* arr = myparser(&buffermsg[0], cli,0);
        struct room rm;
        rm.id = arr[0];
        rm.pw = arr[1];

    if (rm.id.length() > 256 | rm.pw.length() > 256)
    {
        perror("Invalid Room");
    }
    else
    {

        int exist = 0;
        for (room i : rooms)
        {
            if ((rm.id.compare(i.id) == 0))
            {
                exist = 1;

                if((rm.pw.compare(i.pw)) == 0)
                    addRoom(cli, i);
                else
                    exist = -1;
            
                break;
            }
        }
        struct room nw;
        if (exist == 0)
        {
            nw.id = rm.id;
            nw.pw = rm.pw;
            nw.myClients.push_back(cli);
            rooms.push_back(nw);

            addRoom(cli, rooms.at(rooms.size() - 1));
            join_message(cli.fd,1);
        }

        else if ( exist == -1){
            join_message(cli.fd,-1);
        }
        else join_message(cli.fd,1);
    }
}


void sendMessage(uint8_t *buffermsg, uint8_t *bufferheader, client cli){
	string* arr = myparser(&buffermsg[0], cli, 1);
	client c;
	c.name = arr[0];
	c.fd = -1;
	string message = arr[1];

	if(c.name.size()<256){
		for(client a: clients){
			if(a.name.compare(c.name) == 0){
				c.fd = a.fd;
				c.myRoom = a.myRoom;
			}

		}
	}
	else{
		msg_ack(cli.name, c, message, bufferheader, -1);
	}

	if(c.fd == -1){
		msg_ack(cli.name, c, message, bufferheader, -1);

	}
	else if(c.fd != -1 && message.size()<65536){
		msg_ack(cli.name, c, message, bufferheader, 1);
		
	}
	else{
		msg_ack(cli.name, c, message, bufferheader, 1);
	}


        

}

//********************************************************************************************
//***************************** Processing the commands ************************************** 
//********************************************************************************************






void processCommand(int clientSock, client cli, uint8_t command, uint8_t *buffermsg,uint8_t *bufferheader)
{
    struct room rm;
    string* arr;
    uint8_t *p = bufferheader;
     int i =0; 
    switch (command)
    {
    //join room
    case 0x03:
        joinroom(&buffermsg[0], cli);
        break;
    //leave
        // case 0x06:;
    //users
        // case 0x0c:;
    //rooms
        // case 0x09:;
    //nickname
        // case 0x0f:;
    //message
    case 0x12:
     
     printf("header ---gbjhguj---- \n");
     while (i < 8)
         printf("%02x ", p[i++]);

     sendMessage(buffermsg, bufferheader, cli);
     //sender(clientSock, def_msg(), 2);
     break;
    default:
        ARGP_ERR_UNKNOWN;
        break;
    }
}

// recieve_header(int csock, client cli){

// }

void recmsg(int clientSock, client cli)
{
    int msgSize, n, val;
    uint8_t command;
    uint8_t *bufferheader = (uint8_t *)malloc(8);
    uint8_t *buffermsg = (uint8_t *)malloc(65536);

    reciever(clientSock, bufferheader, 7);

    // uint8_t *p = bufferheader;
    //  int i =0; 
    //  printf("header ------- \n");
    //  while (i < 7)
    //      printf("%02x ", p[i++]);


    val = 0;
    memcpy(&val, bufferheader, 4); //find size of message
    uint8_t const1 = bufferheader[4];
    uint8_t const2 = bufferheader[5];
    command = bufferheader[6];
    val = ntohl(val);
    reciever(clientSock, buffermsg, val);
    cout<< "command " << command <<endl;
    // printf("%04x ", command);

    printf("\n");
    if (command != 155)
    {
        int i =0; 
        // printf("Command ------- \n");
        // while (i < val)
        //  printf("%02x ", p[i++]);

        // printf("\n");
        uint8_t *p = buffermsg;
        while (i < val)
            printf("%02x ", p[i++]);

        printf("\n");
        processCommand(clientSock, cli, command, buffermsg,bufferheader);
    }
    else
    {
        uint8_t *k = buffermsg;
        int i = 0;

        // printf("Recieved for greeting ------- \n");
        // while (i < val)
        //     printf("%02x ", k[i++]);

        // printf("\n");
        default_ack(clientSock, cli, const1, const2);
    }
}

//********************************************************************************************
//************************** Create Sockets, Parse, make new clients ************************* 
//********************************************************************************************


//creates new client and give new name
int newClient(int servSock)
{
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    // Wait for a client to connect
    int sock = accept(servSock, (struct sockaddr *)&clientAddr, &len);
    if (sock < 0)
    {
        perror("new client is stupid");
        exit(1);
    }
    int count = 0, flag = -1;
    regex reg("^rand[0-9]+$");

    struct client nw;
    for (client i : clients)
    {

        if (flag == -1 && (i.fd == -1 || i.name.empty()))
        {
            i.fd = sock;
            i.name = "rand" + to_string(count);
            flag = 1;
            nw = i;
        }
        else if (regex_match(i.name, reg))
        {
            count++;
        }
    }

    if (flag == -1)
    {
        nw.fd = sock;
        nw.name = "rand" + to_string(count);
        clients.push_back(nw);
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);
    recmsg(sock, nw);

    return sock;
}

//create sockets for incoming clients and poll
void create(int port)
{

    // Create socket for incoming connections
    int servSock, new_socket; // Socket descriptor for server
    vector<pollfd> poll_sockets; //vector of socket descriptors


    //TCP Socket made
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }
    struct pollfd servfd;
    servfd.fd = servSock; //main sock
    servfd.events = POLLIN | POLLPRI;

    poll_sockets.push_back(servfd); //first fd pushed which is the server socket
    fcntl(servSock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in servAddr; // Local address
    int add_len = sizeof(servAddr);
    memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
    servAddr.sin_family = AF_INET;                // IPv4 address family
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    servAddr.sin_port = htons(port);              // Local port
    
    int n = bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if (n < 0)
    {
        perror("bind() failed");
        exit(1);
    }
    // Mark the socket so it will listen for incoming connections
    if (listen(servSock, SOMAXCONN) < 0)
    {
        perror("listen() failed");
        exit(1);
    }

    cout << "Listening on port " << port << endl;

    int check_new, i = 0;

    while (1)
    {
        i++;
        int incomeClientFlag, t_out = 0;

        for(int i = 0; i < poll_sockets.size(); i++){
            poll_sockets[i].events = POLLIN | POLLPRI;
            poll_sockets[i].revents = 0;
        }

        int p = poll(&poll_sockets[0], poll_sockets.size(), t_out);

        if(p<0)
        {
            perror("poll() failed");
            exit(1);
        }
        else{

            check_new = poll_sockets[0].revents & POLLIN;

            if (check_new)
            {
                struct pollfd cli;
                cli.fd = newClient(servSock); //create new client
                // servfd.events = POLLIN | POLLPRI;
                cli.events = POLLIN;
                poll_sockets.push_back(cli);
            }
            else
            {
                // cout<<"yo"<<endl;
                struct client cli;
                for(int i = 1; i < poll_sockets.size(); i++){
                    // / cout<<"yes"<<endl;
                    
                    int othr_new = poll_sockets[i].revents & POLLIN;
                    if (othr_new){
                        for (client a : clients)
                        {
                            if (a.fd == poll_sockets[i].fd)
                            {
                                recmsg(a.fd, a);
                                break;
                            }
                        }
                    }
                }   
            }
        }
    }
}

int argCheck(int argc, char *argv[])
{
    int port;
    if (argc != 3 || !strcmp(argv[1], "p"))
    {
        perror("Error");
        exit(1);
    }

    port = atoi(argv[2]);
    if (port == 0 || port <= 1024 /* port is invalid */)
    {
      exit(1);
    }
    return port;
}


int main(int argc, char *argv[])
{
    int port = argCheck(argc, &(*argv));
    create(port); //Step 1

    return 1;
}
