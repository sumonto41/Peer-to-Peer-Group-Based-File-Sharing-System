#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "sha1.h"
#include <iostream>
#include <fstream>
using namespace std;
#define PORT 8080
unordered_map<string, string> sha_path;
unordered_map<string, string> sha_piece;
int server_fd;
string ghash, gname;
struct di{
	int piece;
	string path;
	int port;
};
struct sockaddr_in address;
int addrlen = sizeof(address);
vector<string> split(string cmd) {
	vector<string> v;
	string temp="";
	for(int i=0; i<cmd.size(); i++) {
		if(cmd[i]!=' ') temp+=cmd[i];
        if(cmd[i]==' '|| i==cmd.size()-1) {
			if(temp.size()) {
				v.push_back(temp);
				temp="";
			}
		}
	}
	return v;
}
string name(string str) {
	for(int i=str.size()-1; i>=0; i--) {
		if(str[i]=='/') {
			return str.substr(i+1, str.size()-i-1);
		}
	}
	return str;
}
void * d_manager(void * ptr) {
	struct di d=*(struct di *)ptr;
	// cout<<"#hash#"<<ghash<<"#"<<endl;
	// cout<<"#piece#"<<d.piece<<"#"<<endl;
	// cout<<"#port#"<<d.port<<"#"<<endl;
	// cout<<"#path#"<<d.path<<"#"<<endl;
	string hash=ghash;
	//connecting to given port.
	int x=d.port;
	int sock2 = 0, client_fd2;
	struct sockaddr_in serv_addr2;
	//char* hello = (char *)"Hello from client 1";
	if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		pthread_exit(NULL);
	}
	serv_addr2.sin_family = AF_INET;
	//cout<<"#"<<x<<"#"<<endl;
	serv_addr2.sin_port = htons(x);

	// Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr2.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
		pthread_exit(NULL);
	}

	if ((client_fd2=connect(sock2, (struct sockaddr*)&serv_addr2,sizeof(serv_addr2)))< 0) {
		printf("\nConnection Failed \n");
		pthread_exit(NULL);
	}
	//cout<<"#CONNECTED#"<<endl;
	string msg=hash;
	msg+=" "+to_string(d.piece);
	string temp=to_string((msg.size()+1499)/1500);
	// cout<<"#temp#"<<temp<<"#"<<endl;
	send(sock2, temp.c_str(), temp.size(), 0);
	sleep(1);
	for(int i=0; i<(msg.size()+1499)/1500; i++){
		temp=msg.substr(i*1500, min((int)msg.size()-i*1500, 1500));
		send(sock2, temp.c_str(),temp.size(),0); 
	}
	char buffer[1500]={0};
	int valread=read(sock2, buffer, 1500);
	int k=stoi((string)buffer);
	// cout<<"#K#"<<k<<endl;
	string data="";
	for(int i=0; i<k; i++){
		char buff[1500]={0};
        valread = read(sock2, buff, 1500);
		data+=(string)buff;
	}
	//cout<<"#data#"<<data<<"#"<<endl;
	if(hash.substr(d.piece*20, 20)!=sha1(data).substr(0, 20)){
		cout<<"HASH MISMATCH"<<endl;
	}
	else{
		// cout<<"#PATH#"<<d.path+gname<<"#"<<endl;
		ofstream dst((d.path+gname).c_str());
		const int buffer_size=(1<<19);
		dst.seekp(d.piece*buffer_size, ios::beg);
		dst<<data;
		dst.close();
	}
	pthread_exit(NULL);
}
void * manage_host(void * ptr){
	int new_socket=*(int *)ptr;
	while(1){
		char buffer[1500]={0};
    	int valread = read(new_socket, buffer, 1500);
		vector<string> command=split((string) buffer);
		if(command[0]=="stop")
			break;
		else if(command[0]=="pieces"){
			//cout<<command<<endl;
			int k=stoi(command[1]);
			//cout<<"#K="<<k<<"#"<<endl;
			string hash="";
			while(k--){
                char buff[1500]={0};
                int valread = read(new_socket, buff, 1500);
                hash+=(string)buff;
            }
			string msg=to_string((sha_piece[hash].size()+1499)/1500);
			//cout<<"siz: "<<msg<<endl;
			send(new_socket, msg.c_str(), msg.size(), 0);
			sleep(1);
			msg=sha_piece[hash];
			//cout<<msg<<endl;
			for(int i=0; i<(msg.size()+1499)/1500; i++){
				string temp=msg.substr(i*1500, min((int)msg.size()-i*1500, 1500));
				send(new_socket, temp.c_str(),temp.size(),0); 
			}
			//cout<<"sent"<<endl;
		}
		else{
			int k=stoi(command[0]);
			string msg="";
			for(int i=0; i<k; i++){
				char buff[1500]={0};
				valread=read(new_socket, buff, 1500);
				msg+=(string)buff;
			}
			vector<string> message=split((string)msg);
			string hash=message[0];
			int piece=stoi(message[1]);
			fstream src(sha_path[hash].c_str());
			const int buffer_size=(1<<19);
  			char buffer[buffer_size];
			src.seekg(piece*buffer_size, ios::beg);
			src.read(buffer, buffer_size);
			string data=(string) buffer;
			msg=to_string((data.size()+1499)/1500);
			send(new_socket, msg.c_str(), msg.size(),0);
			sleep(1);
			for(int i=0; i<(data.size()+1499)/1500; i++){
				string temp=data.substr(i*1500, min((int)data.size()-i*1500, 1500));
				send(new_socket, temp.c_str(),temp.size(),0); 
			}
		}
	}
	pthread_exit(NULL);
}
void * manage_requests(void * ptr){
	pthread_t T[1000];
    int i=0;
	while(1){
		int new_socket;
		if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        int * p=(int *)malloc(sizeof(int));
        *p=new_socket;
        pthread_create(&T[i++], NULL, manage_host, p);
        if(i==999){
            for(int k=0; k<=i; k++) pthread_join(T[k], NULL);
        }
	}
	pthread_exit(NULL);
}
int main(int argc, char const* argv[])
{
	//connecting to tracker
	int sock = 0, valread, client_fd;
	struct sockaddr_in serv_addr;
	char* hello = (char *)"Hello from client 1";
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
		return -1;
	}

	if ((client_fd
		= connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
		< 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	//socket to listen.

	int opt = 1;
	
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\nsocket failed\n");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		printf("\nsetsockopt\n");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	//cout<<"#"<<stoi((string)argv[1])<<"#"<<endl;
	address.sin_port = htons(stoi((string)argv[1]));
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address))< 0) {
		printf("\nbind failed\n");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		printf("\nlisten failed\n");
		exit(EXIT_FAILURE);
	}
	pthread_t T;
	pthread_create(&T, NULL, manage_requests, (void *)1);
	int flag;	
	while(1){
		flag=1;
		string s;
		getline(cin, s);
		vector<string> cmd=split(s);
		if(cmd[0]=="upload_file"){
			string fpath=cmd[1];
			fstream src(fpath.c_str());
			const int buffer_size=(1<<19);
  			char buffer[buffer_size];
			string hash="";
			int cnt=0;
			while(src.good()){
				src.read(buffer, buffer_size);
				hash+=sha1(buffer).substr(0, 20);
				cnt++;
			}
			for(int i=0; i<cnt; i++){
				sha_piece[hash]+='1';
			}
			//cout<<"#"<<hash.substr(0, 100)<<"#"<<endl;
			sha_path[hash]=fpath;
			s+=" "+to_string((hash.size()+1499)/1500);
			//while(s.size()%1500) s+=' ';
			send(sock, s.c_str(), s.size(), 0);
			sleep(1);
			for(int i=0; i<(hash.size()+1499)/1500; i++){
				string temp=hash.substr(i*1500, min((int)hash.size()-i*1500, 1500));
				send(sock, temp.c_str(),temp.size(),0); 
			}
			//cout<<"META SENT"<<endl;
		}
		else if(cmd[0]=="create_user"){
			s+=" "+(string)argv[1];
			send(sock, s.c_str(), s.size(), 0);
		}
		else if(cmd[0]=="download_file"){
			flag=0;
			//cout<<"#"<<s<<"#"<<endl;
			string fpath=cmd.back();
			string name=cmd[2];
			if(fpath.back()!='/')fpath+='/';
			//cout<<"#"<<fpath<<"#"<<endl;
			s="";
			for(int i=0; i<cmd.size()-1; i++) s+=cmd[i]+" ";
			s.pop_back();
			//cout<<"#"<<s<<"#"<<endl;
			send(sock, s.c_str(), s.size(), 0);// sending download command to tracker.
			char buffer[1500]={0};
			valread=read(sock, buffer, 1500);
			int k=stoi(split((string)buffer)[0]);
			//cout<<"#"<<k<<"#"<<endl;
			string msg="";
			for(int i=0; i<k; i++){// receiving hash and ports
				char buff[1500]={0};
				valread=read(sock, buff, 1500);
				msg+=(string)buff;
			}
			//cout<<"#"<<msg<<"#"<<endl;
			vector<string> hash_prt=split((string)msg);
			string hash=hash_prt[0];
			vector<int> ports;
			for(int i=1; i<hash_prt.size(); i++) {
				ports.push_back(stoi(hash_prt[i]));
			}
			//for(auto x:ports)cout<<x<<" ";
			//cout<<endl;
			unordered_map<int, int> freq;
			unordered_map<int, int> src_peer;
			for(auto x: ports){
				//connect to that port
				int sock2 = 0, client_fd2;
				struct sockaddr_in serv_addr2;
				//char* hello = (char *)"Hello from client 1";
				if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					printf("\n Socket creation error \n");
					return -1;
				}
				serv_addr2.sin_family = AF_INET;
				//cout<<"#"<<x<<"#"<<endl;
				serv_addr2.sin_port = htons(x);

				// Convert IPv4 and IPv6 addresses from text to binary
				// form
				if (inet_pton(AF_INET, "127.0.0.1", &serv_addr2.sin_addr)
					<= 0) {
					printf(
						"\nInvalid address/ Address not supported \n");
					return -1;
				}

				if ((client_fd2=connect(sock2, (struct sockaddr*)&serv_addr2,sizeof(serv_addr2)))< 0) {
					printf("\nConnection Failed \n");
					return -1;
				}
				//fetch piece availability
				string msg="pieces "+to_string((hash.size()+1499)/1500);
				while(msg.size()%1500) msg+=' ';
				send(sock2, msg.c_str(), msg.size(), 0);
				for(int i=0; i<(hash.size()+1499)/1500; i++){
					string temp=hash.substr(i*1500, min((int)hash.size()-i*1500, 1500));
					send(sock2, temp.c_str(),temp.size(),0); 
				}
				char buff[1500]={0};
				valread=read(sock2,buff, 1500);
				k=stoi(split((string)buff)[0]);
				string str="";
				//cout<<"K: "<<k<<endl;
				for(int i=0; i<k; i++){// receiveing piece info
					char buff[1500]={0};
					valread=read(sock2, buff, 1500);
					str+=(string)buff;
				}
				for(int i=0; i<str.size(); i++){
					if(str[i]=='1'){
						freq[i]++;
						src_peer[i]=x;
					}
				}
			}
			vector<pair<int, int>> seq;
			for(auto x:freq){
				seq.push_back({x.second, x.first});
			}
			sort(seq.begin(), seq.end());
			//for(auto x: seq)cout<<x.first<<" "<<x.second<<endl;
			vector<pthread_t> thrds;
			ghash=hash;
			gname=name;
			//cout<<ghash<<endl;
			for(int i=0; i<seq.size(); i++){
				//cout<<"even ONCE"<<endl;
				struct di d;
				//cout<<"allocated"<<endl;
				d.piece=seq[i].second;
				//cout<<"#piece#"<<d.piece<<"#"<<endl;
				d.port=src_peer[seq[i].second];
				//cout<<"#port#"<<d.port<<"#"<<endl;
				d.path=fpath;
				//cout<<"#path#"<<d.path<<"#"<<endl;
				pthread_t Di;
				pthread_create(&Di, NULL, d_manager, (void *)&d);
				thrds.push_back(Di);
				if(thrds.size()==999){
					for(auto x: thrds){
						reverse(thrds.begin(), thrds.end());
						pthread_join(thrds.back(), NULL);
						thrds.pop_back();
					}
				}
			}
			cout<<"\033[7mDOWNLOADED\033[0m"<<endl;
			
		}
		else
			send(sock, s.c_str(), s.size(), 0);
		if(s=="exit") break;
		char buffer[1024] = { 0 };
		if(flag){
			valread=read(sock, buffer, 1024);
			printf("%s", buffer);
		}
	}
	// send(sock, hello, strlen(hello), 0);
	// printf("Hello message sent\n");
	// valread = read(sock, buffer, 1024);
	// printf("%s\n", buffer);

	// closing the connected socket
	pthread_join(T, NULL);
	close(client_fd);
	return 0;
}
