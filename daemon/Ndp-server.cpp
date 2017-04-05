//
// Created by youngwb on 4/5/17.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>

namespace nfd{
    namespace gateway{

#define PORT			38888			//端口号
#define IP_FOUND 		"Npd"		//IP发现命令
#define IP_FOUND_ACK 	"Npd_ACK"	//IP发现应答命令

/***** 服务器监听广播 *******************************************************************************************/
int ServerListenBroadcast(void)
{
    int ret = -1;
    int sock = -1;
    socklen_t len = -1;
    char buf[20];
    fd_set readfd;
    struct timeval timeout;
    struct sockaddr_in local_addr;		//本地地址
    struct sockaddr_in from_addr;		//客户端地址

    //socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(0 > sock)
    {
        perror("client socket err");
        return -1;
    }

    //init local_addr
    len = sizeof(struct sockaddr_in);
    memset(&local_addr, 0, sizeof(struct sockaddr_in));	//清空内存内容
    local_addr.sin_family = AF_INET;					//协议族
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);		//本地地址
    local_addr.sin_port = htons(PORT);					//监听端口

    //bind
    ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if(0 > ret)
    {
        perror("server1 bind err");
        goto _out;
    }

    while(1)
    {
        //文件描述符集合清零
        FD_ZERO(&readfd);
        //将套接字文件描述符加入读集合
        FD_SET(sock, &readfd);

        //超时时间
        timeout.tv_sec = 2;			//秒
        timeout.tv_usec = 0;		//微秒

        //监听套接字sock是否有数据可读
        ret = select(sock+1, &readfd, NULL, NULL, NULL);
        switch(ret)
        {
            case -1:
                printf("select error!\n");
                goto _out;
            case 0:
                printf("time out!\n");
                break;
            default:
            {
                //sock套接口有数据可读
                if( FD_ISSET(sock, &readfd) )
                {
                    //recieve
                    ret = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&from_addr, &len);
                    if(0 > ret)
                    {
                        perror("server recieve err");
                        goto _out;
                    }
//                    printf("server register : %s\n", buf);

                    //如果与IP_FOUND吻合
                    if( strstr(buf, IP_FOUND) )
                    {
                        //send
                        ret = sendto(sock, IP_FOUND_ACK, strlen(IP_FOUND_ACK) + 1, 0, (struct sockaddr *)&from_addr, len);
                        if(0 > ret)
                        {
                            perror("server send err");
                            goto _out;
                        }
                        printf("server register %s\n", inet_ntoa(from_addr.sin_addr));
//						goto _out;	//退出
                    }

                }

            }
        }
    }

    //close
    close(sock);
    return 0;
    _out:
    close(sock);
    return -1;
}

    }
}

///***** 主函数 ****************************************************************************************************/
//int main(void)
//{
//    if( ServerListenBroadcast() )
//    {
//        printf(" server listen broadcast failed!\n");
//        return -1;
//    }
//    return 0;
//}
