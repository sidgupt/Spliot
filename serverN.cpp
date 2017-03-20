


#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <fstream>
#include <map>
#include <algorithm>

#define MAXPENDING 50
#define BUFFSIZE 2048
#define MAX 2048

using namespace std;

map<string,string> user_pass_map;
map<string,string> client_server_command;
map<string,int> command_params;
map<string,int> logged_in_users; 


void Error(char *mess) 
{
    perror(mess);
    exit(1);
}

void sighandler(int signum)
{
   cout<<"Caught signal "<<signum<<", coming out...\n";
   exit(1);
}

void setup(char inputBuffer[], char *args[], int *background)
{
    const char s[4] = " \t\n";
    char *token;
    token = strtok(inputBuffer, s);
    int i = 0;
    while( token != NULL)
    {
        args[i] = token;
        i++;
        token = strtok(NULL,s);
    }
    args[i] = NULL;
}

void userExit(string userName, int sock)    
{
	if(!userName.empty())
		logged_in_users[userName]--;
	userName.clear();
	send(sock, "User Exited!!", BUFFSIZE, 0);		
	//userAuth = false;
        exit(1);
}

int create_socket(int port)
{
	int listenfd;
	struct sockaddr_in dataservaddr;


	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		Error("Error: Problem in creating the data socket");
		exit(2);
	}

dataservaddr.sin_family = AF_INET;
dataservaddr.sin_addr.s_addr = htonl(INADDR_ANY);
dataservaddr.sin_port = htons(port);

	if ((bind (listenfd, (struct sockaddr *) &dataservaddr, sizeof(dataservaddr))) <0) {
		Error("Error: Problem in binding the data socket");
		exit(2);
	}

 listen (listenfd, 1);

return(listenfd);

}

void unixFunction(int sock, char *args[])
{
int pipefd[2],lenght;

if(pipe(pipefd))
    Error("Error: Failed to create pipe");

pid_t pid = fork();
char path[MAX];

if(pid==0)
{
    close(1); // close the original stdout
    dup2(pipefd[1],1); // duplicate pipfd[1] to stdout
    close(pipefd[0]); // close the readonly side of the pipe
    close(pipefd[1]); // close the original write side of the pipe
    execvp(args[0],args); // finally execute the command

}
else
    if(pid>0)
    {
	close(pipefd[1]);

	if(strcmp(args[0], "cd")!=0)
	{
	memset(path,0,MAX);

	while(lenght=read(pipefd[0],path,MAX-1)){

	    if(send(sock,path,strlen(path),0) != strlen(path) ){
		Error("Error: Send Fail");
	    }
	memset(path,0,MAX);
	}
	}
	close(pipefd[0]);
    }
    else
    {
	Error("Error: Unexpected Error!");
	exit(0);
    }

}


void HandeClient(int sock){

string userName;
bool userAuth= false;
char buffer[BUFFSIZE] ={0};
//char ex[BUFFSIZE]="exit";
int received = -1;
char data[MAX] = {0};
//memset(data,0,MAX);
char currDir[MAX];
string initialbuf;
char finalbuf[BUFFSIZE] ={0};


while(1) { 

    memset(buffer,0,BUFFSIZE);
    memset(data,0,BUFFSIZE);
    signal(SIGINT, sighandler);

    int flag =0;
    if((received = recv(sock, buffer,BUFFSIZE,0))<0){

        Error("Error: Failed");
    }
 	if(strlen(buffer)==0)    // ^C case
	{
		userExit(userName, sock);		
	}

	std::map<string,string>::iterator iter;
	initialbuf = buffer;
	int position = initialbuf.find(" ");
	string strr = initialbuf.substr(0, position);

	iter = client_server_command.find(strr);

	if(iter != client_server_command.end())
	{

	    int params_given = std::count(initialbuf.begin(), initialbuf.end(), ' ');
	    int count =0, found=0;
	    while(found>=0)
	    {
		found = initialbuf.find(" -",found+1,2);   	// to check the number of parameters passed
		if(found > 0)
			count++;
	    }
	    params_given = params_given - count;		// params given = space count - " -" count.
	    
	    if(params_given != command_params[strr])
		{
			send(sock, "Error: Number of pamameters is incorrect.", MAX, 0);
			continue;
		}
	    if(strr.compare(client_server_command[strr])!=0)
		{
			string finalstr = "";
			finalstr.append(client_server_command[strr]);
			if(position >0)
				finalstr.append(initialbuf.substr(position));
			memset(buffer,0,BUFFSIZE);
			strcpy(buffer, finalstr.c_str());
			received = finalstr.size();
			
		}
	    buffer[received] = '\0';
	    
	    strcat (data,  buffer);
		if((int)data[0]==0)
		{
			exit(1);
		}	
	    if (strcmp(data, "exit")==0) // this will force the code to exit
		{
			userExit(userName, sock);
		}
	  
		char *args[100];
		setup(data,args,0);
		if(strcmp(args[0], "login")==0)
		{
			    
			    map<string,string>::iterator it;
			    it = user_pass_map.find(args[1]);
				if (it == user_pass_map.end())
				{
					send(sock,"Error: Wrong Username!!",MAX,0);
				}
				else
				{
				    string passw_dum;
				    passw_dum = user_pass_map[args[1]];
				    char *passw =  &passw_dum[0];
				    send(sock,"Enter Password as pass $password!",MAX,0);
				    memset(buffer,0,BUFFSIZE);
				    recv(sock, buffer,BUFFSIZE,0);
				    char *psswd= strtok (buffer," ");
				    psswd = strtok(NULL, " ");
				    if(strcmp(passw,psswd)==0)
					{
						userAuth = true;
						userName.append(args[1]);
						logged_in_users[userName]++;
						send(sock,"Successful Login!",MAX,0);
					}
				    else
					{
						send(sock,"Error: Wrong Credentials!!",MAX,0);
					}
				} 			  
		}

		if(userAuth==true)
		{
			if(strcmp(args[0], "cd")==0)
			{
					int chd = chdir(args[1]); 
					if(chd<0){
						send(sock,"Error: No such directory",BUFFSIZE,0);
					}
					else{
						getcwd(currDir, sizeof(currDir));
						send(sock,currDir,MAX,0);
						memset(currDir,0,MAX);
					}
	
			}
			else if(strcmp(args[0], "whoami")==0)
			{
					send(sock, userName.c_str(), BUFFSIZE, 0);
			}
			else if(strcmp(args[0], "w")==0)
			{
					string userList="";
					for(map<string,int>::iterator it = logged_in_users.begin(); it != logged_in_users.end(); ++it)
					{
						if(it->second > 0)
						{
							userList.append(it->first);
						}
					}
					send(sock, userList.c_str(), BUFFSIZE, 0);
			}
			else if (strcmp(args[0], "logout")==0)
			{
					logged_in_users[userName]--;
					send(sock, "User Successfully logged out", BUFFSIZE, 0);
					exit(1);
			}
		
			else
			{
					unixFunction(sock, args);
			}
		}
		else
		{
			send(sock,"Error: Login required to run the command",MAX,0);
		}
	}
	else
	{
		buffer[received] = '\0';
		strcat (data,  buffer);
		char *args[100];
		setup(data,args,0);
		unixFunction(sock, args);		
	}
}
}

int main(int argc, char const *argv[])
{
	ifstream input( "spliot.conf" );
	char *value;
	char *pass;
	char *user_Name;
	char *command;
	char *commandName;
	int params;
	string str;	
	int port_num;
	for(string line; getline( input, line );)
	{
		if(line.size()==0)
			continue;
		char * c =  &line[0];
		char * dummy = &line[0];
		//cout<<"CC: "<<line<<" Size: "<<line.size()<<endl;
		value = strtok (c," ");
		//cout<<"USER: "<<user<<endl;
		if(strcmp (value, "user")==0)
		{	
			user_Name = strtok (NULL, " ");
			pass = strtok (NULL, " ");
			user_pass_map[user_Name]=pass;
			logged_in_users[user_Name]=0;
		}
		if(strcmp(value, "command")==0)
		{
			command = strtok (NULL, " ");
			if(line.find('$'))
			{
				params = std::count(line.begin(), line.end(), '$');
				commandName = strtok(NULL, "$");
				str = commandName;
				if(commandName[strlen(commandName)-1]==' ')
				{
					
					str.erase(strlen(commandName)-1,1);
				}
			}
			else
			{
				commandName = strtok(NULL, "\n");
			}

			client_server_command[command]=str;
			command_params[command]=params;
			str.clear();
		}
		if(strcmp(value, "port")==0)
		{
			port_num = atoi(strtok(NULL, " "));
		}
	}

    int serversock,clientsock, pid;
    struct sockaddr_in echoserver, echoclient;
    unsigned int clientlen = sizeof(echoclient);
    signal(SIGPIPE, SIG_IGN); // this will allow code to handle SIGPIPE instead of crashing
    if((serversock = socket(PF_INET, SOCK_STREAM,IPPROTO_TCP))<0){
        Error("Error: Socket Creation failed.");
    }
    memset(&echoserver,0,sizeof(echoserver));
    echoserver.sin_family = AF_INET;
    echoserver.sin_addr.s_addr= htonl(INADDR_ANY);
    echoserver.sin_port = htons(port_num);

    if(bind(serversock, (struct sockaddr *) & echoserver,sizeof(echoserver))<0){
        Error("Error: Binding Failed");
    }
    if(listen(serversock,MAXPENDING)<0){
        Error("Error: Max client limit reached.");
    }
    while(1)
    {
        if((clientsock = 
            accept(serversock,(struct sockaddr *) &echoclient,
                                &clientlen))<0){
            Error("Error: Unable to accept from client");
        }
	pid = fork();

	if (pid ==0)
	{

		close(serversock);
		HandeClient(clientsock);
		//continue;
	}
	else
	{

        	//HandeClient(clientsock);
		close(clientsock);
	}        
    }

    return 0;
}
