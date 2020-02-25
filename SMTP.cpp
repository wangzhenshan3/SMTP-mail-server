#include "stdafx.h"
#pragma comment(lib,"wsock32.lib") 

#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<WinSock2.h>
#include<sstream>
#include<time.h>

using namespace std;

char mail_from[4096];
char rcpt_to[5][4096];
char _data[4096];
char imf[4096];
void main_Client(SOCKET, int);
void send_mail();
int rcpt_addr(char *a);
int gettime(char *,char *);

//SYSTEMTIME now;

void main()
{
	char str1[50];
	char str2[50];

	clock_t time;

	WORD A = MAKEWORD(1, 1); //加载套接字库
	WSADATA B;  //储存调用WSAStartup函数返回的Windows Sockets初始化信息
	int err;
	err = WSAStartup(A, &B);  //根据A要求的版本初始化Winsock服务

	if (err != 0)   // Tell the user that we couldn't find a useable 
	{		  	 // winsock.dll. 
		return;
	}

	if (LOBYTE(B.wVersion) != 1 || HIBYTE(B.wVersion) != 1)
	{
		WSACleanup(); //检查socket版本，不符合则释放分配资源		return;
	}

	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0); //创建用于监听客户端的套接字	
	SOCKADDR_IN addrSrv;	//作为服务器端的socket地址
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");	// Internet address 

	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(25);  //服务器端端口号
	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); //绑定套接字
	listen(sockSrv, 5); //将套接字设为监听模式，准备接受客户请求

	SOCKADDR_IN addrClient;  //客户端地址
	int len = sizeof(SOCKADDR);
	char *sendBuf[] = {
		"220 LX's SMTP Ready\r\n",
		"250 LX's server|250 mail|250 PIPELINING\r\n",
		"250 OK\r\n",
		"250 OK\r\n",
		"354 Start mail input;end with <CR><LF>.<CR><LF>\r\n",
		"250 OK\r\n",
		"250 OK\r\n",
		"QUIT\r\n",
		"550 Invalid User\r\n" }; //发送标示符

	char tempbuf1[4096] = "";

	while (1)  //等待客户请求
	{
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len); //队列非空则sockSrv抽取第一个链接，否则阻塞调用进程
		FILE *fp = NULL;
		memset(str1, 0, 50);
		memset(str2, 0, 50);
		gettime(str1, str2);
		fp = fopen(str1, "w+");
		char recvBuf[4096] = ""; //接收客户端SMTP指令

		memset(rcpt_to, 0, sizeof(rcpt_to));

		send(sockConn, sendBuf[0], strlen(sendBuf[0]), 0);  //向已经连接的套接字sockConn发送连接建立信息：220

		printf("%s\n", str2);
		//printf("发送方IP为%s,发送方端口为：%d\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port) + 1);
		printf("发送方IP为:10.122.203.133, 发送方端口为：%d\n", ntohs(addrClient.sin_port) + 1);


		recv(sockConn, recvBuf, sizeof(recvBuf), 0); //接收数据 EHLO
		fprintf(fp, "%d--", time = clock());
		fprintf(fp, "%s\n", recvBuf); //将数据写入文件

		memset(recvBuf, 0, sizeof(recvBuf)); //将recvBuf前4096个字节用字符'0'替换
		send(sockConn, sendBuf[1], strlen(sendBuf[1]), 0); // send:250 OK
		recv(sockConn, recvBuf, sizeof(recvBuf), 0); //recv:MAIL FROM:<...>
		if (rcpt_addr(recvBuf) < 0) 
		{
			send(sockConn, sendBuf[8], strlen(sendBuf[8]), 0);
			closesocket(sockConn);
			fclose(fp);
			continue; 
		}//send:550

		memcpy(mail_from, recvBuf, sizeof(recvBuf));
		fprintf(fp, "%d--", time = clock());
		fprintf(fp, "%s\n", recvBuf);
		memset(recvBuf, 0, sizeof(recvBuf));
		send(sockConn, sendBuf[2], strlen(sendBuf[2]), 0); //send:250 OK

		recv(sockConn, recvBuf, sizeof(recvBuf), 0); //recv: RCPT TO:<....>
		if (rcpt_addr(recvBuf) < 0) 
		{ 
			send(sockConn, sendBuf[8], strlen(sendBuf[8]), 0);
			closesocket(sockConn); 
			fclose(fp); 
			continue; 
		}//send:550 
		memcpy(rcpt_to[0], recvBuf, sizeof(recvBuf));
		fprintf(fp, "%d--", time = clock());
		fprintf(fp, "%s\n", recvBuf);
		memset(recvBuf, 0, sizeof(recvBuf));
		send(sockConn, sendBuf[2], strlen(sendBuf[2]), 0); //send:250 OK
		recv(sockConn, recvBuf, sizeof(recvBuf), 0);//recv:??
		int i = 1;
		strncpy(tempbuf1, recvBuf, 4);
		while ((strcmp(tempbuf1, "RCPT") == 0) && (i < 5))
		{
			if (rcpt_addr(recvBuf) < 0) 
			{ 
				send(sockConn, sendBuf[8], strlen(sendBuf[8]), 0);
				closesocket(sockConn);
				fclose(fp);
				continue;
			}//send:550 
			memcpy(rcpt_to[i], recvBuf, sizeof(recvBuf));
			fprintf(fp, "%d--", time = clock());
			fprintf(fp, "%s\n", recvBuf);
			memset(recvBuf, 0, sizeof(recvBuf));
			send(sockConn, sendBuf[2], strlen(sendBuf[2]), 0); //send:250 OK
			recv(sockConn, recvBuf, sizeof(recvBuf), 0); //recv: RCPT TO:<....>
			strncpy(tempbuf1, recvBuf, 4);
			++i;
		}
		fprintf(fp, "%d--", time = clock());
		fprintf(fp, "%s\n", recvBuf);
		memset(recvBuf, 0, sizeof(recvBuf));
		send(sockConn, sendBuf[4], strlen(sendBuf[4]), 0);//send:354 Start mail input;end with <CR><LF>.<CR><LF>\r\n

		recv(sockConn, recvBuf, sizeof(recvBuf), 0); //recv:DATA
		memcpy(_data, recvBuf, sizeof(recvBuf));
		fprintf(fp, "%s\n", recvBuf);
		memset(recvBuf, 0, sizeof(recvBuf));
		send(sockConn, sendBuf[5], strlen(sendBuf[5]), 0); //send:250 OK

		recv(sockConn, recvBuf, sizeof(recvBuf), 0); //recv:IMF
		memcpy(imf, recvBuf, sizeof(recvBuf));
		//printf("%s\n", recvBuf);
		fprintf(fp, "%s\n", recvBuf);
		memset(recvBuf, 0, sizeof(recvBuf));
		send(sockConn, sendBuf[6], strlen(sendBuf[6]), 0); //send:250 OK

		recv(sockConn, recvBuf, sizeof(recvBuf), 0); //recv: . 
		//printf("%s\n", recvBuf);
		fprintf(fp, "%s\n", recvBuf);
		memset(recvBuf, 0, sizeof(recvBuf));
		send(sockConn, sendBuf[7], strlen(sendBuf[7]), 0); //send:QUIT
		//printf("%s\n", recvBuf);
		fprintf(fp, "%s\n", recvBuf);

		printf("%s\n", mail_from);

		int q = 0;
		while (q < 5 && rcpt_to[q][0] != '\0')
		{
			main_Client(sockConn, q); //调用客户端函数
			printf("%s\n", rcpt_to[q]);
			q++;
		}

		printf("邮件长度：%d 字节\n", strlen(_data));

		closesocket(sockConn); //关闭套接字

		fclose(fp);  //关闭文件指针

	}
	WSACleanup(); //释放分配资源
}

void main_Client(SOCKET sockCo, int i)
{
	WORD A = MAKEWORD(1, 1);
	WSADATA B;
	int err;
	err = WSAStartup(A, &B);

	if (err != 0) { return; }
	if (LOBYTE(B.wVersion) != 1 || HIBYTE(B.wVersion) != 1)
	{
		WSACleanup(); return;
	}

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrClient;

	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(25);

	struct hostent *host;  //主机信息

	
	char *SendBuf[] = {
		"EHLO smtp.qq.com\r\n",
		"AUTH LOGIN\r\n",
		"YTk2MjYxMjkwNw==\r\n",//邮箱名的Base64编码
		"NjgxMERFTkdjZw==\r\n",//密码的Base64编码
		"DATA\r\n",
		"\r\n.\r\n",
		"QUIT"
	};

	char arecvBuf[4096] = "";
	char tempbuf[3] = "";

	host = NULL;

	int j = 0;

	j = rcpt_addr(rcpt_to[i]);
	if (j == 1)
	{
		host = gethostbyname("smtp.163.com");
	}//163邮箱
	else if (j == 2)
	{
		host = gethostbyname("smtp.126.com");
	}//126邮箱
	else if (j == 3)
	{
		host = gethostbyname("freemx1.sinamail.sina.com.cn");
	}//sina邮箱
		else if (j == 4)
	{
		host = gethostbyname("mx1.bupt.edu.cn");
	}//bupt邮箱
	else
	{
			;
	}//无法提供服务

	memcpy(&addrClient.sin_addr.S_un.S_addr, host->h_addr_list[0], host->h_length); //将获取的主机IP地址复制到客户端网络地址.32位无符号IPV4地址 
	connect(sockClient, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));  //连接套接字  

	printf("接收方IP：%s，接收方端口号：%d\n", inet_ntoa(addrClient.sin_addr) , ntohs(addrClient.sin_port));

	memset(tempbuf, 0, sizeof(tempbuf));
	memset(arecvBuf, 0, sizeof(arecvBuf));  //初始化arecvBuf

	recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:220 OK

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, SendBuf[0], strlen(SendBuf[0]), 0); //send:EHLO
	recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:250 OK
	strncpy(tempbuf, arecvBuf, 3);
	if (strcmp(tempbuf, "250") != 0)
	{
		send(sockCo, arecvBuf, strlen(arecvBuf), 0);
	}

	if (j == 1 || j == 2)
	{
		memset(arecvBuf, 0, sizeof(arecvBuf));
		send(sockClient, SendBuf[1], strlen(SendBuf[1]), 0); //send:AUTH LOGIN
		recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:334
		strncpy(tempbuf, arecvBuf, 3);
		if (strcmp(tempbuf, "334") != 0) 
		{ 
			send(sockCo, arecvBuf, strlen(arecvBuf), 0); 
		}

		memset(arecvBuf, 0, sizeof(arecvBuf));
		send(sockClient, SendBuf[2], strlen(SendBuf[2]), 0); 
		recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:334
		strncpy(tempbuf, arecvBuf, 3);
		if (strcmp(tempbuf, "334") != 0) 
		{ 
			send(sockCo, arecvBuf, strlen(arecvBuf), 0);
		}

		memset(arecvBuf, 0, sizeof(arecvBuf));
		send(sockClient, SendBuf[3], strlen(SendBuf[3]), 0); //
		recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //rec:235
		strncpy(tempbuf, arecvBuf, 3);
		if (strcmp(tempbuf, "235") != 0) { send(sockCo, arecvBuf, strlen(arecvBuf), 0); }

	}

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, mail_from, strlen(mail_from), 0); //send:MAIL FROM:<...>
	recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:250 OK
	strncpy(tempbuf, arecvBuf, 3);
	if (strcmp(tempbuf, "250") != 0)
	{
		send(sockCo, arecvBuf, strlen(arecvBuf), 0);
	}

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, rcpt_to[i], strlen(rcpt_to[i]), 0); //send:RCPT TO:<....>
	recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:250 OK
	strncpy(tempbuf, arecvBuf, 3);
	if (strcmp(tempbuf, "250") != 0)
	{
		send(sockCo, arecvBuf, strlen(arecvBuf), 0);
	}

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, SendBuf[4], strlen(SendBuf[4]), 0); //send: DATA
	recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:354
	strncpy(tempbuf, arecvBuf, 3);
	if (strcmp(tempbuf, "354") != 0)
	{
		send(sockCo, arecvBuf, strlen(arecvBuf), 0);
	}

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, _data, strlen(_data), 0);  //send:DATA fragment, ...bytes

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, imf, strlen(imf), 0);  //send:imf fragment

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, SendBuf[5], strlen(SendBuf[5]), 0); //send: . 
	recv(sockClient, arecvBuf, sizeof(arecvBuf), 0);  //recv:250 OK
	strncpy(tempbuf, arecvBuf, 3);
	if (strcmp(tempbuf, "250") != 0)
	{
		send(sockCo, arecvBuf, strlen(arecvBuf), 0);
	}

	memset(arecvBuf, 0, sizeof(arecvBuf));
	send(sockClient, SendBuf[6], strlen(SendBuf[6]), 0); //send: QUIT

	
	closesocket(sockClient);

	WSACleanup();
}



int gettime(char *str1, char *str2)
{
	/*stringstream sstr;
	SYSTEMTIME st;
	GetSystemTime(&st);
	sstr << "log-" << st.wYear << "/" << st.wMonth << "/" << st.wDay << "/" << st.wHour + 8 << "/" << st.wMinute << ".txt";
	sstr >> str;
	return 0;*/
	time_t now;
	struct tm* curtime;
	FILE *fp = NULL;
	now = time(NULL);
	curtime = localtime(&now);
	sprintf(str1, "LOG-%04d-%02d-%02d-%02d-%02d.txt", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_hour, curtime->tm_min);
	sprintf(str2, "%04d年%02d月%02d日%02d时%02d分", curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_hour, curtime->tm_min);

	return 0;
}

int rcpt_addr(char *a)
{
	char add[40] = { 0 };
	char tmp[30] = { 0 };
	char name[100] = { 0 };
	char name_buf[4][12]{
		"163.com",
		"126.com",
		"sina.com",
		"bupt.edu.cn" };
	int m = 0;
	int n = 0;
	int y = 0;
	while (a[m] != '>')
	{
		if (n == 0 && a[m] == ':')//找到：，并向后跳跃三个字符，然后开始拷贝
		{
			m = m + 3;
			n = 1;
			continue;
		}
		else if (n == 1)
		{
			add[y++] = a[m];
		}
		else
		{
			;
		}
		m++;
	}
	add[m] = '\0';

	int i = 0, j = 0;
	for (i; i < strlen(add); i++)
	{
		if (add[i] == '@' && i < strlen(add) - 1)//找到@ 
		{
			break;
		}
		else if (i == strlen(add) - 1)
		{
			//printf("ERROR: 1\n");
			return -1;
		}
		else
		{
			tmp[i] = add[i];
			continue;
		}
	}

	i++;
	tmp[i] = '\0';
	for (i; i < strlen(add); i++)
	{
		name[j++] = add[i];
	}
	//name[j] = '\0';
	for (i = 0; i < 4; i++)
	{
		if (strcmp(name, name_buf[i]) == 0)
		{
			return (i + 1);
		}
	}
	return -2;
	}

