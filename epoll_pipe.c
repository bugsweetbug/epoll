#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>

#define SIZE 10

void err(const char *str)
{
	perror(str);
	exit(1);
}


int main()
{
	int i;
	int pfd[2];
	pid_t pid;
	char buf[SIZE];
	char ch = 'a';
	pipe(pfd);

	pid = fork();
	if(pid < 0)
		err("fork error");
	if(pid == 0)
		{
			close(pfd[0]);
			while(1)
				{
					for(i = 0; i < SIZE/2; i++)
						buf[i] = ch;
					buf[i-1] = '\n';
					ch++;
					for(; i < SIZE; i++)
						buf[i] = ch;
					buf[i-1] = '\n';
					ch++;
					write(pfd[1], buf, SIZE);
					sleep(3);
				}
			close(pfd[1]);
		}
	if(pid > 0)
		{
			struct epoll_event events;
			struct epoll_event evt[10];
			int efd, ret, n;
			close(pfd[1]);

			efd = epoll_create(10);
			//events.events = EPOLLIN;/*默认水平触发, 缓冲区有数据epoll_wait就会返回*/
			events.events = EPOLLIN | EPOLLET;
			events.data.fd = pfd[0];
			ret = epoll_ctl(efd, EPOLL_CTL_ADD, pfd[0], &events);
			if(ret < 0)
				err("epoll_ctl error");
			while(1)
				{
					epoll_wait(efd, evt, 10, -1);
					if(evt[0].data.fd == pfd[0])
						{
							n = read(pfd[0], buf, SIZE/2);
							write(STDOUT_FILENO, buf, n);

						}

				}
			close(pfd[0]);
		}
	return 0;
}