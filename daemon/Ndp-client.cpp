//
// Created by youngwb on 4/5/17.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include<unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "nwd.hpp"
#include "Ndp.hpp"
#include "table/routetable-entry.hpp"

namespace nfd{
    namespace gateway{

std::vector<std::string> EthName={"eth0","wlan0"};   //网络接口名字

/***** 获取本地Mac地址 ******************************************************************************************/
int Nwd::GetLocalMac(int sock, char *vmac,std::string ifname)
{
    struct sockaddr sa;
    struct ifreq ifr;
    char mac[6];

    strncpy(ifr.ifr_name, ifname.data(), IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    //获取Mac地址
    memset(mac, 0, sizeof(mac));
    if (0 > ioctl(sock, SIOCGIFHWADDR, &ifr))
    {
        perror("ioctl");
        return -1;
    }

    memcpy(&sa, &ifr.ifr_addr, sizeof(struct sockaddr_in));
    memcpy(mac, sa.sa_data, 6);
    sprintf(vmac,"%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("MacAddr : %s", vmac);

    return 0;
}


/***** 获取接口广播地址 *****************************************************************************************/
int Nwd::GetBroadcastAddr(int sock, struct sockaddr_in *broadcast_addr,std::string ifname)
{
    struct ifreq ifr;

    //获取网络接口字符串名字
    strncpy( ifr.ifr_name, ifname.data(), ifname.size()+1 );
    //获取网络接口的广播地址
    if(0 > ioctl(sock, SIOCGIFBRDADDR, &ifr))
    {
        perror("ioctl");
        return -1;
    }
    memcpy(broadcast_addr, &ifr.ifr_broadaddr, sizeof(struct sockaddr_in));

    return 0;
}


/***** 设置广播 *************************************************************************************************/
int Nwd::SetBroadcast(int sock, struct sockaddr_in *broadcast_addr)
{
    int ret = -1;
    int so_broadcast = 1;						//设置连接侦测超时时间为1秒
    broadcast_addr->sin_port = htons(PORT);		//设置广播端口

    //set broadcast
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));
    if(0 > ret)
    {
        perror("client set broadcast err");
        return -1;
    }

    return 0;
}


void Nwd::RoutingtableUpdate()
{
    for(RouteTable_Type::iterator itr=route_table.begin();itr!= route_table.end();)   //先将路由表中的邻居内容清空
    {
        if(itr->second.get_reachstatus() == gateway::RouteTableEntry::neighbor)
        {
            itr=route_table.erase(itr);
        }
        else
            ++itr;
    }


    for(Neighbor_Type::iterator itr=neighbors_list.begin();itr!= neighbors_list.end();++itr)
    {
        route_table.insert(std::make_pair(itr->first,RouteTableEntry(itr->first, 0, gateway::RouteTableEntry::neighbor,gateway::RouteTableEntry::notsending,itr->second)));
    }

}

/***** 客户端广播 ***********************************************************************************************/
int Nwd::ClientBroadcast(void)
{
    int ret = -1;
    int sock = -1;
    char buf[1024];
    fd_set readfd;
    struct timeval timeout;
    struct sockaddr_in broadcast_addr;	//本地地址
    struct sockaddr_in from_addr;		//客户端地址
    socklen_t len = sizeof(struct sockaddr_in);

    time_t current_timestamp;
    std::time(&current_timestamp);
    std::string ndp_discover=IP_FOUND+to_string(longitude)+"/"+to_string(latitude)+"/"+std::to_string(current_timestamp);
    //socket
    std::vector<int> sock_vec;  //保存sock
    std::unordered_multimap<std::string,std::string> ribItemName;
    while(true)
    {
        for(auto itr:ribItemName)
        {
            ribUnregisterPrefix(itr.first,itr.second);
        }

        for(auto itr:EthName)
        {
            sock = socket(AF_INET, SOCK_DGRAM, 0);
    //        std::cout<<"sock is "<<sock<<std::endl;
            if(0 > sock)
            {
                perror("client socket err");
    //            return -1;
            }

            //get broadcast addr
            if( GetBroadcastAddr(sock, &broadcast_addr,itr) )
            {
                printf("get  %s broadcast addr failed!\n",itr.data());
                close(sock);
                continue;
    //            goto _out;
            }

            //set broadcast
            if( SetBroadcast(sock, &broadcast_addr) )
            {
                printf("set %s broadcast failed!\n",itr.data());
                close(sock);
                continue;
    //            goto _out;
            }

            //send

            std::cout<<"client sned :"<<ndp_discover<<std::endl;
            ret = sendto(sock, ndp_discover.data(), ndp_discover.size() + 1, 0, (struct sockaddr *)&broadcast_addr, len);
            sock_vec.push_back(sock);
        }


        if(0 > ret)
        {
            perror("server send err");
            goto _out;
        }

        //文件描述符集合清零
        FD_ZERO(&readfd);
        //将套接字文件描述符加入读集合
        int maxsock=0;
        for(unsigned int i=0;i<sock_vec.size();i++) {
            if(maxsock<sock_vec[i])
                maxsock=sock_vec[i];
            FD_SET(sock_vec[i], &readfd);
        }

        //超时时间
        timeout.tv_sec = 5;			//秒
        timeout.tv_usec = 0;		//微秒

        boost::this_thread::sleep(boost::posix_time::seconds(2));  //线程等待发送NDP和NDP确认
        ribItemName.clear();
        //监听套接字sock是否有数据可读，超时为5秒
        while (true) {
            ret = select(maxsock + 1, &readfd, NULL, NULL, &timeout);
            switch (ret) {
                case -1:
                    printf("select error!\n");
                    goto _out;
                case 0:
                    printf("time out!\n");
                    break;
                default: {
                    //sock套接口有数据可读
                    for (auto sock : sock_vec) {
                        if (FD_ISSET(sock, &readfd)) {
                            //recieve
                            ret = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &from_addr, &len);
                            std::string remote_ip(inet_ntoa(from_addr.sin_addr));
                            if (0 > ret) {
                                perror("server recieve err");
                                goto _out;
                            }
//                    printf("client find : %s\n", buf);
                            std::cout << "client receive ack: " << buf << std::endl;
                            //如果与IP_FOUND吻合
                            if (strncmp(buf, IP_FOUND_ACK, 9) == 0) {
                                std::string point_x, point_y;
                                getPointLocation(buf, point_x, point_y);

                                std::string remote_name = std::string("udp://") + remote_ip;
                                std::cout << "remote_name:" << remote_name << std::endl;
                                std::string fibitemname("/nfd/" + point_x + "/" + point_y);
                                ribRegisterPrefix(fibitemname, remote_name);
                                ribItemName.insert(std::make_pair(fibitemname,remote_name));
//                        printf("client find %s\n", inet_ntoa(from_addr.sin_addr));
                            }

                        }
                    }

                }
            }
            if(ret ==0)
                break;
        }
        for(auto sock : sock_vec) {
            close(sock);
        }
        sock_vec.clear();
        neighbors_list.clear();  //清空邻居表
        getNeighborsCoordinate();  //邻居表初始化、更新
        printNeighborsTable();  //打印邻居表
        RoutingtableUpdate();  //更新路由表

        boost::this_thread::sleep(boost::posix_time::seconds(10));  //周期发送
    }

_out:
    close(sock);
    return -1;
}



    }
}

//***** 主函数 ****************************************************************************************************/
//int main(void)
//{
//    if( ClientBroadcast() )
//    {
//        printf(" client broadcast failed!\n");
//        return -1;
//    }
//
//    return 0;
//}
