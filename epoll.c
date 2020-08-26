#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#define MAXLINE 1024
#define SERV_IP "127.0.0.1"
#define SERV_PORT 8000
#define OPEN_MAX 1024

void err(const char *str)
{
	perror(str);
	exit(1);
}

int main()
{
	int listenfd, connfd, sockfd, retval, ret, res;
	int n, i, j, num = 0;
	ssize_t nready, efd;
	char buf[MAXLINE], str[INET_ADDRSTRLEN];
	socklen_t clilen;

	struct sockaddr_in serv_addr, clie_addr;
	struct epoll_event tep, ep[OPEN_MAX];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0)
		err("socket error");

	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr);

	ret = bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret < 0)
		err("bind error");
	ret = listen(listenfd, 128);
	if (ret < 0)
		err("listen error");
	efd = epoll_create(OPEN_MAX);										/*注册epfd，此时epfd = 3*/
	if(efd < 0)
		err("epoll_create error");

	tep.events = EPOLLIN;												/*设置events为读事件*/
	tep.data.fd = listenfd;												/*将socket返回的listenfd加入监听队列*/

	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);		/*将socket返回的listenfd加入监听队列*/
	if(res != 0)
		err("epoll_ctl error");
	for( ; ; )
		{
			nready = epoll_wait(efd, ep, OPEN_MAX, -1);
			if(nready < 0)
				err("epoll_wait error");
			for(i = 0; i < nready; i++)
				{
					if(!ep[i].events & EPOLLIN)						/*如果没有读事件发生*/
						continue;

					if(ep[i].data.fd == listenfd)						/*读事件是listenfd发出，意味有客户端连接*/
						{
							clilen = sizeof(clie_addr);
							connfd = accept(listenfd, (struct sockaddr *)&clie_addr, &clilen);
							if (connfd < 0)
								err("accept error");
							printf("received form %s at PORT %d\n",
							       inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, str, sizeof(str)),
							       ntohs(clie_addr.sin_port));

							tep.events = EPOLLIN;						
							tep.data.fd = connfd;						/*将新连接上的connfd加入到读监听队列*/
							res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
							if(res != 0)
								err("epoll_ctl error");

						}
					else														/*读事件由其中一个connfd发出，有数据写入*/
						{
							sockfd = ep[i].data.fd;						/*将ep队列中读事件发生的fd拿出，方便操作*/
							n = read(sockfd, buf, sizeof(buf)); 	/*读客户端发来的数据*/
							if(n == 0)
								{												/*如果read返回0，客户端关闭, 将fd移出监听队列*/
									res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, &tep);
									if(res < 0)
										err("epoll_ctl_del error");
									close(sockfd);							/*并关闭文件描述符*/
									printf("Client[%d] closed\n",sockfd);
								}
							else if(n < 0)
								{												/*read出错, 同样处理*/
									err("read error");
									res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, &tep);
									close(sockfd);
								}
							else
								{
									for(j = 0; j < n; j++)
										buf[j] = toupper(buf[j]);
									write(sockfd, buf, n);
									write(STDOUT_FILENO, buf, n);
								}
						}
				}
		}


	close(listenfd);
	close(efd);
	return 0;
}