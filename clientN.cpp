    #include <stdio.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <fstream>
    #include<iostream>
    #define BUFFSIZE 2048

using namespace std;
	
    int sock;

    void Error(char *mess) 
    {
        perror(mess); 
	//printf("ee\n");
        exit(1); 
    }

    void sighandler(int signum)
    {
	printf("Caught signal %d, coming out...\n", signum);
	send(sock, "exit", 5, 0);
	exit(1);
    }

    int main(int argc, char *argv[]) 
    {
        struct sockaddr_in echoserver;
        char buffer[BUFFSIZE];
        unsigned int echolen;
        int received = 0;
	

        if (argc != 3 && argc !=5) 
        {
            fprintf(stderr, "USAGE: TCPecho <server_ip> <word> <port>\n");
            exit(1);
        }


        if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
        {
            Error("Failed to create socket");
        }
        memset(&echoserver, 0, sizeof(echoserver));
        echoserver.sin_family = AF_INET;
        echoserver.sin_addr.s_addr = inet_addr(argv[1]);
        echoserver.sin_port = htons(atoi(argv[2]));
        if (connect(sock,(struct sockaddr *) &echoserver,sizeof(echoserver)) < 0) 
        {
            Error("Failed to connect with server");
        }

	if (argc == 5)
	{
		ifstream infile( argv[3] );
		ofstream outfile( argv[4] );
		for(string line; getline( infile, line );)
		{
				if(line.size()==0)
					continue;
				echolen = line.size();
					
				if (send(sock, line.c_str(), echolen, 0) != echolen) 
				{
				    outfile<<"Mismatch in number of sent bytes";
				}
				while (received < echolen) 
				{
				    int bytes = 0;
				    /* recv() from server; */
				    if ((bytes = recv(sock, buffer, echolen, 0)) < 1) 
				    {
					outfile<<"Failed to receive bytes from server";
				    }
				    received += bytes;
				    buffer[bytes] = '\0';
				    
				    outfile<< buffer;
				}
				int bytes = 0;
		
				do {
				    buffer[bytes] = '\0';
				    outfile<< buffer<<endl;
				} while((bytes = recv(sock, buffer, BUFFSIZE-1, 0))>=BUFFSIZE-1);
				buffer[bytes] = '\0';
				outfile<< buffer<<endl;
				outfile<<"\n";
			

		}  
	}
	else
	{

		char s[100];
	    while(1) { // to repeat the whole process until exit is typed
		fgets(s,100,stdin);
		s[strlen(s)-1]='\0'; //fgets doesn't automatically discard '\n'
		echolen = strlen(s);

		/*  send() from client; */
		if (send(sock, s, echolen, 0) != echolen) 
		{
		    Error("Mismatch in number of sent bytes");
		}
		if(strcmp(s,"exit") == 0)
		{
			send(sock, "exit", 5, 0);
			exit(0);
		}
		fprintf(stdout, "Message from server: ");
		while (received < echolen) 
		{
		    int bytes = 0;
		    /* recv() from server; */
		    if ((bytes = recv(sock, buffer, echolen, 0)) < 1) 
		    {
		        Error("Failed to receive bytes from server");
		    }
		    received += bytes;
		    buffer[bytes] = '\0';
		    /* Assure null terminated string */
		    fprintf(stdout,"%s", buffer);
		}
		int bytes = 0;
	// this d {...} while block will receive the buffer sent by server
		do {
		    buffer[bytes] = '\0';
		    printf("%s\n", buffer);
		} while((bytes = recv(sock, buffer, BUFFSIZE-1, 0))>=BUFFSIZE-1);
		buffer[bytes] = '\0';
		printf("%s\n", buffer);
		printf("\n");
	    }
	}
    }
