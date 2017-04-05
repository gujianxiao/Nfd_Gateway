//
// Created by youngwb on 4/5/17.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "nwd.hpp"
#include "Ndp.hpp"

namespace nfd{
    namespace gateway{




/***** 服务器监听广播 *******************************************************************************************/
int Nwd::ServerListenBroadcast(void)
{
    int ret = -1;
    int sock = -1;
    socklen_t len = -1;
    char buf[1024];
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
                    std::cout<<"server receive : "<<buf<<std::endl;
                    if( strncmp(buf, IP_FOUND,5) ==0 )
                    {
                        //send
                        std::string point_x,point_y;
                        getPointLocation(buf,point_x,point_y);
//                        std::cout<<"pointx: "<<point_x<<"pointy: "<<point_y<<std::endl;
                        std::string remote_ip(inet_ntoa(from_addr.sin_addr));
                        bool flag=true;
                        for(auto itr:ethface_map)  //不能绑定本地端口
                        {

                            if(remote_ip == itr.second)
                            {
                                flag=false;
                                break;
                            }

                        }
                        if(flag == true) {  //不是本地端口
                            time_t current_timestamp;
                            std::time(&current_timestamp);
                            std::string ndp_discover_ack=IP_FOUND_ACK+to_string(longitude)+"/"+to_string(latitude)+"/"+std::to_string(current_timestamp);
                            std::cout<<"server send ack :"<<ndp_discover_ack<<std::endl;

                            //发送ＡＣＫ
                            ret = sendto(sock, ndp_discover_ack.data(), ndp_discover_ack.size() + 1, 0, (struct sockaddr *)&from_addr, len);
                            if(0 > ret)
                            {
                                perror("server send err");
                                goto _out;
                            }
//                            printf("server register %s\n", inet_ntoa(from_addr.sin_addr));

                            std::string remote_name = std::string("udp://") + remote_ip;
                            std::cout << "remote_name:" << remote_name << std::endl;

                            ribRegisterPrefix("/nfd/"+point_x+"/"+point_y,remote_name);
                        }
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
