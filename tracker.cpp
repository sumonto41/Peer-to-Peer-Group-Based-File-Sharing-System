#include<bits/stdc++.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include <iostream>
#include <fstream>
using namespace std;
#define PORT 8080
struct group{
    string owner;
    unordered_map<string, int> members;
    set<string> joinrequest;
    unordered_map<string, string> file_sha;
    unordered_map<string, set<string>> files_peer;
};
vector<group> groups;
unordered_map<string, int> grpi; 
unordered_map<string, string> id_pass;
unordered_map<string, int> peer_port;
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
void *manage_host(void * ptr){
    //sleep(10);
    bool xt= false;
    int new_socket=*(int *)ptr;
    while(1){
        if(xt) break;
        char buffer[1500]={0};
        int valread = read(new_socket, buffer, 1500);
        vector<string> command=split((string)buffer);
        if(command[0]=="exit") break;
        if(command[0]=="create_user"){
            string id=command[1];
            string pass=command[2];
            string prt=command[3];
            ////////cout<<"#ID#"<<id<<"#PASS#"<<pass<<"#"<<endl;
            id_pass[id]=pass;
            peer_port[id]=stoi(prt);
            // fstream cred("credentials.txt", fstream::app);
            // cred<<id<<" "<<pass<<"\n";
            // cred.close();
            string msg="\033[7mUSER CREATED\033[0m\n";
	        send(new_socket, msg.c_str(), msg.size(), 0);
        }
        else if(command[0]=="login"){ // handle case "login"
            string id=command[1];
            string pass=command[2];
            ////////cout<<"#ID#"<<id<<"#PASS#"<<pass<<"#"<<endl;
            if(id_pass[id]==pass){
                string msg="\033[7mLOGIN SUCCESSFUL\033[0m\n";
	            send(new_socket, msg.c_str(), msg.size(), 0);
                while(1){
                    char buff[1024]={0};
                    valread = read(new_socket, buff, 1024);
                    command=split((string)buff);
                    if(command[0]=="logout"){
                        string msg="\033[7mLOGOUT SUCCESSFUL\033[0m\n";
	                    send(new_socket, msg.c_str(), msg.size(), 0);
                        break;
                    }
                    else if(command[0]=="login"){
                        string msg="\033[7mALREADY LOGGED IN AS "+id+"\033[0m\n";
	                    send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="create_group"){
                        string gid=command[1];
                        if(grpi[gid]==0){
                            groups.push_back(group());
                            groups.back().owner=id;
                            groups.back().members[id]=1;
                            grpi[gid]=groups.size();
                            string msg="\033[7mGROUP CREATED\033[0m\n";
	                        send(new_socket, msg.c_str(), msg.size(), 0);
                        }
                        else{
                            string msg="\033[7mGROUP WITH SAME ID ALREADY EXISTS\033[0m\n";
	                        send(new_socket, msg.c_str(), msg.size(), 0);
                        }
                    }
                    else if(command[0]=="list_groups"){
                        string msg="\033[7m"+to_string(grpi.size())+" GROUP(S):\033[0m\n";
                        for(auto x:grpi){
                            msg+="\033[7m"+x.first+"\033[0m\n";
                        }
	                    send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="join_group"){
                        string gid=command[1];
                        groups[grpi[gid]-1].joinrequest.insert(id);
                        string msg="\033[7mJOIN REQUESTED\033[0m\n";
	                    send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="leave_group"){
                        string gid= command[1];
                        groups[grpi[gid]-1].members.erase(id);
                        for(auto x:groups[grpi[gid]-1].files_peer)
                            x.second.erase(id);
                        groups[grpi[gid]-1].owner=(*groups[grpi[gid]-1].members.begin()).first;
                        string msg="\033[7mGROUP LEFT SUCCESSFULY\033[0m\n";
	                    send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="list_requests"){
                        string gid=command[1];
                        string msg="\033[7m"+to_string(groups[grpi[gid]-1].joinrequest.size())+" PENDING REQUESTS:\033[0m\n";
                        for(auto x: groups[grpi[gid]-1].joinrequest){
                            msg+="\033[7m"+x+"\033[0m\n";
                        }
                        send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="accept_request"){
                        string gid=command[1];
                        string uid=command[2];
                        if(id==groups[grpi[gid]-1].owner){
                            groups[grpi[gid]-1].members[uid]=1;
                            groups[grpi[gid]-1].joinrequest.erase(uid);
                            string msg="\033[7mREQUEST ACCEPTED\033[0m\n";
                            send(new_socket, msg.c_str(), msg.size(), 0);
                        }
                        else{
                            string msg="\033[7mONLY OWNER CAN ACCEPT JOIN REQUEST\033[0m\n";
                            send(new_socket, msg.c_str(), msg.size(), 0);
                        }
                    }
                    else if(command[0]=="list_files"){
                        string gid=command[1];
                        string msg="\033[7m"+to_string(groups[grpi[gid]-1].file_sha.size())+" FILES:\033[0m\n";
                        for(auto x:groups[grpi[gid]-1].file_sha){
                            msg+="\033[7m"+x.first+"\033[0m\n";
                        }
                        send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="upload_file"){
                        string fname=name(command[1]);
                        //////////cout<<"fname: "<<fname<<endl;
                        string gid=command[2];
                        //////////cout<<"gid: "<<gid<<endl;
                        string packets=command[3];
                        ////////cout<<"packets: "<<packets<<endl;
                        int k=stoi(packets);
                        string hash="";
                        while(k--){
                            char buff[1500]={0};
                            int valread = read(new_socket, buff, 1500);
                            hash+=(string)buff;
                        }
                        ////////cout<<"#"<<hash.substr(0, 100)<<"#"<<endl;
                        groups[grpi[gid]-1].file_sha[fname]=hash;
                        groups[grpi[gid]-1].files_peer[fname].insert(id);
                        string msg="\033[7mFILE UPLOADED\033[0m\n";
                        send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="download_file"){
                        string gid=command[1];
                        ////////cout<<"#"<<gid<<"#"<<endl;
                        string fname=command[2];
                        ////////cout<<"#"<<fname<<"#"<<endl;
                        string msg=groups[grpi[gid]-1].file_sha[fname];
                        for(auto x:groups[grpi[gid]-1].files_peer[fname]){
                            msg+=" "+to_string(peer_port[x]);
                        }
                        while((msg.size()%1500)!=0) msg+=' ';
                        ////////cout<<"#"<<msg<<"#"<<endl;
                        string temp=to_string(msg.size()/1500);
                        while(temp.size()<1500) temp+=' ';
                        send(new_socket, temp.c_str(), temp.size(), 0);
                        for(int i=0; i<(msg.size()+1499)/1500; i++){
				            temp=msg.substr(i*1500, min((int)msg.size()-i*1500, 1500));
				            send(new_socket, temp.c_str(),temp.size(),0); 
			            }
                        msg="\033[7mDOWNLOADING...\033[0m\n";
                        //send(new_socket, msg.c_str(), msg.size(),0); 
                    }
                    else if(command[0]=="debug"){
                        string gid=command[1];
                        cout<<"#####################"<<endl;
                        cout<<gid<<":"<<endl;
                        cout<<"OWNER: "<<groups[grpi[gid]-1].owner<<endl;
                        cout<<"MEMBERS:"<<endl;
                        for(auto x:groups[grpi[gid]-1].members)
                            cout<<x.first<<" "<<peer_port[x.first]<<endl;
                        cout<<"REQUESTS:"<<endl;
                        for(auto x:groups[grpi[gid]-1].joinrequest){
                            cout<<x<<endl;
                        }
                        cout<<"FILES-PEERS:"<<endl;
                        for(auto x:groups[grpi[gid]-1].files_peer){
                            cout<<x.first<<" : ";
                            for(auto y: x.second){
                                cout<<y<<";";
                            }
                            cout<<endl;
                            cout<<groups[grpi[gid]-1].file_sha[x.first]<<endl;
                        }
                        cout<<"#####################"<<endl;
                        string msg="\033[7mDEBUGGED\033[0m\n";
                        send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                    else if(command[0]=="exit"){
                        xt=true;
                        break;
                    }
                    else{
                        string msg="\033[7mINVALID COMMAND\033[0m\n";
                        send(new_socket, msg.c_str(), msg.size(), 0);
                    }
                }
            }
            else{
                string msg="\033[7mINVALID USER ID OR PASSWORD\033[0m\n";
                send(new_socket, msg.c_str(), msg.size(), 0);
            }
        }
        else{
            string msg="\033[7mCREATE USER OR LOGIN FIRST!\033[0m\n";
            send(new_socket, msg.c_str(), msg.size(), 0);
        }
    }
    pthread_exit(NULL);
}
int main(int argc, char const* argv[])
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address))< 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
    // fstream cred("credentials.txt");
    // string id;
    // while(cred>>id){
    //     string pass;
    //     cred>>pass;
    //     id_pass[id]=pass;
    // }
    // cred.close();
    pthread_t T[1000];
    int i=0;
    int cnt=0;
    while(true){
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        int * p=(int *)malloc(sizeof(int));
        *p=new_socket;
        pthread_create(&T[i++], NULL, manage_host, p);
        if(i==999){
            for(int k=0; k<=i; k++) pthread_join(T[k], NULL);
            i=0;
        }
    }
	// closing the connected socket
	// closing the listening socket
	return 0;
}
