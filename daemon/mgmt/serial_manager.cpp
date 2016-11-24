//
// Created by xiayula on 16-5-31.
//
#include "serial_manager.hpp"
#include <termios.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <vector>
//#include "pending_interest.hpp"
//#include "ndn_data.hpp"
#include <boost/shared_ptr.hpp>
#include <signal.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

//extern std::vector <ndn::gateway::pending_interest> interest_list;

namespace nfd {
    namespace gateway {
        serial_manager::serial_manager(boost::condition_variable_any& m_data_ready) : datagotflag(0), total_read(0), top(0), bottom(0), data_flag(0),
                                           thread_flag(1),data_ready(m_data_ready),globe_timestamp(0){
            try {
                fdusb = open(USB_PATH_PORT, O_RDWR);
                if (fdusb == -1) {
                    perror("open");
                }
                int ret = set_opt(115200, 8, 'N', 1);
                if (ret == -1) {
                    perror("set_opt");
                }
                sem_init(&sem_queue, 0, 0);//\u521d\u59cb\u5316\u4fe1\u53f7\u91cf
                sem_init(&sem_interest, 0, 0);//\u521d\u59cb\u5316\u5174\u8da3\u5305\u4fe1\u53f7\u91cf

                memset(topo_tree_node_check, 0, 256);

                topo_head = (BTNode *) malloc(sizeof(BTNode));
                memset(topo_head, 0, sizeof(BTNode));

                scope_name = (char *) malloc(sizeof(char) * 64);

            } catch (std::exception &e) {
                std::cout << e.what() << std::endl;
            }
        }

		bool serial_manager::position_belong_interest(std::string Interest,int point_x,int point_y){
//			std::cout<<Interest<<std::endl;
//			std::cout<<point_x<<std::endl;
//			std::cout<<point_y<<std::endl;
			std::string::size_type x_begin_idx,x_end_idx,y_begin_idx,y_end_idx; //use for find '/'
			x_begin_idx=Interest.find('/');
			x_end_idx=Interest.find('/',x_begin_idx+1);
			y_begin_idx=x_end_idx;
			y_end_idx=Interest.find('/',y_begin_idx+1);
			std::string scope_x=Interest.substr(x_begin_idx+1,x_end_idx-x_begin_idx-1);
			std::string scope_y=Interest.substr(y_begin_idx+1,y_end_idx-y_begin_idx-1);
//			std::cout<<scope_x<<std::endl;
//			std::cout<<scope_y<<std::endl;
			int leftup_x=std::stoi(scope_x.substr(0,scope_x.find(',')));
			int leftup_y=std::stoi(scope_x.substr(scope_x.find(',')+1));
//			std::cout<<leftup_x<<std::endl;
//			std::cout<<leftup_y<<std::endl;
			int rightdown_x=std::stoi(scope_y.substr(0,scope_y.find(',')));
			int rightdown_y=std::stoi(scope_y.substr(scope_y.find(',')+1));
//			std::cout<<rightdown_x<<std::endl;
//			std::cout<<rightdown_y<<std::endl;

			if(point_x>=leftup_x && point_x<=rightdown_x){
				if(point_y>=rightdown_y && point_y<=leftup_y){
//					std::cout<<"TRUE"<<std::endl;
					return true;
				}
			}
			
			return false;
		}

		bool serial_manager::time_belong_interest(std::string Interest,int datatime){
			std::string::size_type In_Start_Timepos,In_Middle_Timepos,In_End_Timepos;
			In_End_Timepos=Interest.rfind('/');
			In_Middle_Timepos=Interest.rfind('/',In_End_Timepos-1);
			In_Start_Timepos=Interest.rfind('/',In_Middle_Timepos-1);
			int endtime=std::stoi(Interest.substr(In_Middle_Timepos+1,In_End_Timepos-In_Middle_Timepos-1));
			int starttime=std::stoi(Interest.substr(In_Start_Timepos+1,In_Middle_Timepos-In_Start_Timepos-1));
//			std::cout<<endtime<<std::endl;
//			std::cout<<starttime<<std::endl;
			if(datatime>=starttime && datatime<=endtime)
				return true;
			return false;
			
		}

		bool serial_manager::datatype_equal_interest(std::string Interest,std::string datatype){
			std::string In_Type=Interest.substr(Interest.rfind('/')+1);
//			std::cout<<In_Type<<std::endl;
			if(In_Type == datatype)
				return true;
			return false;
		}

        void serial_manager::read_data(std::map<std::string,std::set<std::string>>& In_List,std::map<std::string,std::string>& location_map) {

            DEBUG printf("\u3010create listening thread successed!!\u3011\n");
            int i, j;
            int nread;
            //unsigned char start_mark[] = {0x7e,0x45,0x00};
            char *start_p;
            char usbbuf[1024]; //\u4e34\u65f6\u7f13\u5b58\u533a
//            unsigned char name_buf[64];
//            unsigned char content_buf[10];
            char name_buf[64];
            char content_buf[10];
			char data_ret[1024];
            uint16_t content;
            int total_read = 0;
            int datagotflag = 0;

            int move_pos;
            recv_data = (Msg *) malloc(sizeof(Msg));
            total_read = 0;
            while (thread_flag) {
                memset(usbbuf, 0, 1024);

                datagotflag = 0;
                while (total_read < SHORTEST_DATA_FRAME_LEN)//\u6bcf\u6b21\u8bfb\u53d6\u81f3\u5c11DATA_FRAME_LEN\u5b57\u8282\u7684\u6570\u636e(\u6570\u636e\u5305\u957fDATA_FRAME_LEN)
                {
                    nread = read(fdusb, usbbuf + total_read, 1024);//\u8bfbUSB\u4e32\u53e3
                    total_read += nread;
//                    DEBUG printhex_macaddr(usbbuf, total_read, " ");

                }
//                DEBUG printhex_macaddr(usbbuf, total_read, " ");
//                printf("\n");
                for (i = 0; i < total_read - 7; i++)//\u627e\u5230\u6570\u636e\u5305\u6807\u5fd7
                {
                    if (usbbuf[i] == 0x7e && usbbuf[i + 1] == 0x45 && usbbuf[i + 2] == 0x00 && usbbuf[i + 3] == 0x00
                        && usbbuf[i + 4] == 0x00 && usbbuf[i + 5] == 0x00 && usbbuf[i + 6] == 0x01) //TODO \u6570\u636e\u5305\u6807\u5fd7\u600e\u4e48\u89c4\u5b9a\u7684\uff1f\uff1f
                    {
                        datagotflag = 1;
                        //DEBUG printf("total_read=%d, i=%d\n", total_read, i);
                        while (total_read - i < SHORTEST_DATA_FRAME_LEN)//\u6807\u5fd7\u540e\u7684\u6570\u636e\u4e0d\u591fDATA_FRAME_LEN\u5b57\u8282\uff0c\u7ee7\u7eed\u8bfb\u6570\u636e
                        {
                            nread = read(fdusb, usbbuf + total_read, 1024);
                            total_read += nread;
                            //DEBUG printf("total_read=%d, i=%d\n", total_read, i);
                        }
                        start_p = usbbuf + i;//start_p point to the begining of the frame
                        break;
                    }
                }
                if (!datagotflag)//\u6ca1\u627e\u5230\u6807\u5fd7\u4f4d\uff0c\u4e0d\u4e88\u5904\u7406
                {
//                    printf("not found the start flag!\n");

                    for (move_pos = total_read - 1; move_pos >= total_read - 7; move_pos--) {
                        if (usbbuf[move_pos] == 0x7e)
                            break;
                    }
                    if (move_pos < total_read - 7) {
                        total_read = 0;
                        continue;
                    }
                    for (j = move_pos, i = 0; j < total_read; j++, i++) {
                        usbbuf[i] = usbbuf[j];
                    }
                    total_read -= move_pos;
                    continue;
                }
//                DEBUG printf("got the packet and nread=%d\n", total_read);

                if (*(start_p + 10) == DATA)//\u5185\u5bb9\u5305
                {
                    while (total_read < 28 + start_p - usbbuf)//\u6bcf\u6b21\u8bfb\u53d6\u81f3\u5c1128\u5b57\u8282\u7684\u6570\u636e
                    {
                        nread = read(fdusb, usbbuf + total_read, 1024);//\u8bfbUSB\u4e32\u53e3
                        total_read += nread;
                    }
//                    DEBUG printf("Got a data message!\n");
//                    DEBUG printhex_macaddr(start_p, 28, " ");
//                    DEBUG printf("\n");

                    memcpy(recv_data, start_p + 10, sizeof(Msg));
                    if (recv_data->msgType == DATA) {
                        DEBUG printf("Got Sensor data!\n");

                        memset(name_buf, 0, sizeof(name_buf));
                        memset(content_buf, 0, sizeof(content_buf));
                        content = recv_data->data;

                        char content_tmp[16];
                        sprintf(content_tmp, "%hd", content);
                        std::string key;
                        sprintf(name_buf, "/%hd,%hd/%d/",
                                recv_data->msgName.ability.leftUp.x, recv_data->msgName.ability.leftUp.y,
                                recv_data->msgName.time_period.start_time+(int)globe_timestamp);
                        if (recv_data->msgName.dataType == Light) {
                            strcat(name_buf, "light");
                            key = "light";
                        }
                        if (recv_data->msgName.dataType == Temp) {
                            strcat(name_buf, "temp");
                            key = "temp";
                        }
                        if (recv_data->msgName.dataType == Humidity) {
                            strcat(name_buf, "humidity");
                            key = "humidity";
                        }

//						printf("interest name = %s\n", name_buf);
                        sprintf(content_buf, "/%hd", content);
//                      printf("content data = %s", content_buf);
						strcpy(data_ret,name_buf);
						strcat(data_ret,content_buf);
						printf("data_ret is %s\n",data_ret);

//						std::cout<<"In_List size is"<<In_sList.size()<<std::endl;
						for(auto& itr:In_List){
							if(position_belong_interest(itr.first,recv_data->msgName.ability.leftUp.x,
								recv_data->msgName.ability.leftUp.y)){
								if(time_belong_interest(itr.first,recv_data->msgName.time_period.start_time+(int)globe_timestamp)){
									if(datatype_equal_interest(itr.first,key)){
//										std::cout<<"match"<<std::endl;
										itr.second.insert(data_ret);
										std::cout<<"data insert"<<std::endl;
									}
								}	
							}	
						}

						
						

                        //TODO src
//                        std::shared_ptr<interest> ints = std::make_shared<interest>(std::string(name_buf), interest::wifi);

//                        std::shared_ptr<ndn_data> data = std::make_shared<ndn_data>(ints, key, ints->x1, ints->y1, std::string(content_tmp));
//                        std::cout << "\u6536\u5230\u4f20\u611f\u5668\u8282\u70b9\u53d1\u6765\u7684\u6570\u636e\u5305\uff1a " << *data << std::endl;

//                        std::vector<ndn::gateway::pending_interest>::iterator it;
//                        for (it = interest_list.begin(); it != interest_list.end(); it++) {
//                            std::cout << ints->getName() << " == ? " << it->ints.getName() << std::endl;

//                            if (ints->x1 == it->ints.x1 && ints->y1 == it->ints.y1 && ints->x2 == it->ints.x2 &&
//                                ints->y2 == it->ints.y2 &&
//                                ints->start_time >= it->ints.start_time && ints->end_time <= it->ints.end_time) {
//                                std::cout << "insert into data_list" << std::endl;
//                                (it->data_list).push_back(data);
//                            }
//                        }
                        data_flag = 1;
//						data_ready.notify_one();
//						return ;
                    }
                }
                else if (*(start_p + 10) == TOPOLOGY) //\u6536\u5230\u62d3\u6251\u7ed3\u6784\u6570\u636e\u5305
                {
                    while (total_read < 37 + start_p - usbbuf)//\u6bcf\u6b21\u8bfb\u53d6\u81f3\u5c1138\u5b57\u8282\u7684\u6570\u636e
                    {
                        nread = read(fdusb, usbbuf + total_read, 1024);//\u8bfbUSB\u4e32\u53e3
                        total_read += nread;
                    }
                    DEBUG printf("Got a topology message!\n");
                    DEBUG printhex_macaddr(start_p, 37, " ");
                    DEBUG printf("\n");
                    topo_msg *recv_topo = (topo_msg *) malloc(sizeof(topo_msg));
                    memcpy(recv_topo, start_p + 12, sizeof(topo_msg));
                    printf("num=%d\n", recv_topo->num);

                    if (top != (bottom - 1 + PTR_MAX) % PTR_MAX) {
                        queue[top++] = recv_topo;
                        if (top >= PTR_MAX)
                            top = top % PTR_MAX;
                        sem_post(&sem_queue); //\u5524\u9192\u62d3\u6251\u91cd\u5efa\u7ebf\u7a0b
                    }
                }
                else if (*(start_p + 10) == MAPPING) //\u6536\u5230\u8282\u70b9\u4f4d\u7f6e\u5750\u6807
                {
                    while (total_read < 25 + start_p - usbbuf)//\u6bcf\u6b21\u8bfb\u53d6\u81f3\u5c1126\u5b57\u8282\u7684\u6570\u636e
                    {
                        nread = read(fdusb, usbbuf + total_read, 1024);//\u8bfbUSB\u4e32\u53e3
                        total_read += nread;
                    }
                    DEBUG printf("Got a mapping message!\n");
                    DEBUG printhex_macaddr(start_p, 25, " ");
                    DEBUG printf("\n");

                    node_info *node = (node_info *) malloc(sizeof(node_info));
                    memcpy(node, start_p + 12, sizeof(node_info));

                    //\u4f20\u611f\u5668\u53d1\u9001\u56de\u7684\u5730\u7406\u4f4d\u7f6e\u5750\u6807\u53ea\u4f7f\u7528leftUp\u5b58\u50a8
                    nodeID_mapping_table[node->nodeID].coordinate = node->coordinate.leftUp;
                    nodeID_mapping_table[node->nodeID].expire = 5;
                    printf("nodeID=%d->(%d,%d)\n",
                           node->nodeID, nodeID_mapping_table[node->nodeID].coordinate.x,
                           nodeID_mapping_table[node->nodeID].coordinate.y);
					char tmp[64];
					sprintf(tmp,"(%d,%d)",nodeID_mapping_table[node->nodeID].coordinate.x,
                           nodeID_mapping_table[node->nodeID].coordinate.y);
					location_map.insert(std::make_pair(to_string(node->nodeID),tmp));
                    free(node);
                }
                for (move_pos = total_read - 1; move_pos >= total_read - 7; move_pos--) {
                    if (usbbuf[move_pos] == 0x7e)
                        break;
                }
                if (move_pos < total_read - 7) {
                    total_read = 0;
                    continue;
                }
                for (j = move_pos, i = 0; j < total_read; j++, i++) {
                    usbbuf[i] = usbbuf[j];
                }
                total_read -= move_pos;
            }
            return ;
        }

        /**
         * entrence of ndw
         */
        int serial_manager::handle_interest(char *interest) {
            DEBUG printf("process the interest from backbone\n");
            printf("interest = %s\n", interest);

            if (strncmp(interest, "wsn", 3) == 0)  // interest may like "ints/3237,6311/3237,6311/t1/t2/light"
            {
                char arg[7][20] = {0};
                int i = 0, j = 0, count = 0;
                char *interest_msg = strstr(interest, "/");
                //\u63d0\u53d6\u7ecf\u7eac\u5ea6\u5750\u6807\u548c\u8bf7\u6c42\u8d44\u6e90\u7c7b\u522b
                for (i = 0, j = 0; interest_msg[i] != '\0'; i++) {
                    if (interest_msg[i] != ',' && interest_msg[i] != '/' && interest_msg[i] != '(') {
                        while (interest_msg[i] != ',' && interest_msg[i] != '/' && interest_msg[i] != ')' &&
                               interest_msg[i] != '\0')
                            arg[count][j++] = interest_msg[i++];
                        j = 0;
                        count++;
                    }
                }
                interest_name *name = (interest_name *) malloc(sizeof(interest_name));
                memset(name, 0, sizeof(interest_name));
                name->ability.leftUp.x = atoi(arg[0]);
                name->ability.leftUp.y = atoi(arg[1]);
                name->ability.rightDown.x = atoi(arg[2]);
                name->ability.rightDown.y = atoi(arg[3]);
				std::cout<<arg[4]<<" "<<arg[5]<<std::endl;
				std::cout<<"golbe time:"<<globe_timestamp<<std::endl;
				if(atoi(arg[4])>=globe_timestamp  && atoi(arg[5]) >=globe_timestamp){
                	name->time_period.start_time = atoi(arg[4])-(int)globe_timestamp; //transfor local telosb time
               	    name->time_period.end_time = atoi(arg[5])-(int)globe_timestamp;
				}else{
					name->time_period.start_time=0;
					name->time_period.end_time=0;
				}
//				if(name->time_period.start_time <0 || name->time_period.end_time <0)
//					return 0;
				
                if (!strcmp(arg[6], "light"))
                    name->dataType = Light;
                if (!strcmp(arg[6], "temp"))
                    name->dataType = Temp;
                if (!strcmp(arg[6], "humidity"))
                    name->dataType = Humidity;
                DEBUG printf("location is(%d, %d),(%d, %d)\n",
                             name->ability.leftUp.x, name->ability.leftUp.y, name->ability.rightDown.x,
                             name->ability.rightDown.y);
                DEBUG printf("type is %s(value=%d)\n", arg[6], name->dataType);
                DEBUG printf("start_time is %d, end_time is %d\n", name->time_period.start_time,
                             name->time_period.end_time);
                //\u5c01\u88c5tinyOS\u6570\u636e\u5305\uff0c\u901a\u8fc7\u4e32\u53e3\u5c06Interest\u53d1\u9001\u7ed9\u4e0b\u6e38\u4f20\u611f\u5668
                usb_write(name, IN);
                free(name);
                strcpy(scope_name, "ndn:/wsn/");
                strcat(scope_name, interest);
                sem_post(&sem_interest);
            }
            else if (strncmp(interest, "location", 8) == 0) {
                char name_buf[256] = {0};
                char content_buf[1024] = {0};
                char temp_buf[100] = {0};
                strcpy(name_buf, "ndn:/wsn/");
                strcat(name_buf, interest);
                int i;
                for (i = 0; i < 256; i++) {
                    if (nodeID_mapping_table[i].expire >= 0) {
                        sprintf(temp_buf, "%d %d,%d %d\n", i, nodeID_mapping_table[i].coordinate.x,
                                nodeID_mapping_table[i].coordinate.y, nodeID_mapping_table[i].expire);
                        strcat(content_buf, temp_buf);
                    }
                }
                pack_data_content(name_buf, content_buf);
            }
            else if (strncmp(interest, "topo", 4) == 0) {
                char name_buf[256] = {0};
                char content_buf[1024] = {0};
                char temp_buf[1024] = {0};
                char temp[20] = {0};
                strcpy(name_buf, "ndn:/wsn/");
                strcat(name_buf, interest);
                int count = 0;
                if (topo_head->child_num > 0) {
                    int i;
                    for (i = 0; i < topo_head->child_num; i++)
                        topo_tree_traversal(&(topo_head->ptr[0]), &count, temp_buf, temp);
                }
                sprintf(temp, "0 %d\n", count);
                strcat(content_buf, temp);
                strcat(content_buf, temp_buf);
                printf("topo response:%s\n", content_buf);
                pack_data_content(name_buf, content_buf);
            }
            return 0;
        }

        void serial_manager::topo_tree_traversal(BTNode **node, int *count, char *buf, char *temp) {
            int i;
            sprintf(temp, "%d %d\n", (*node)->nodeID, (*node)->parent->nodeID);
            strcat(buf, temp);
            (*count)++;
            for (i = 0; i < (*node)->child_num; i++) {
                topo_tree_traversal(&((*node)->ptr[i]), count, buf, temp);
            }
        }

        /**
         * Cal the CRC Byte
         */
        uint16_t serial_manager::crcByte(uint16_t crc, uint8_t b) {
            crc = (uint8_t)(crc >> 8) | (crc << 8);
            crc ^= b;
            crc ^= (uint8_t)(crc & 0xff) >> 4;
            crc ^= crc << 12;
            crc ^= (crc & 0xff) << 5;
            return crc;
        }

        /**
         * Cal the CRC Val
         */
        uint16_t serial_manager::crccal(unsigned char *buf, int len) {
            uint16_t res = 0;
            int i;

            for (i = 0; i < len; ++i) {
                res = crcByte(res, buf[i]);
            }

            return res;
        }

        /**
         * write the interest into USB
         */
        void serial_manager::usb_write(interest_name *interest, uint16_t type) {
            DEBUG printf("\u3010enter the function usb_write~~\u3011\n");
            int nread;

            tinyosndw_packet *packet = (tinyosndw_packet *) malloc(sizeof(tinyosndw_packet));
            memset((char *) packet, 0, sizeof(tinyosndw_packet));
            packet->head.F = 0x7e;
            packet->head.P = 0x44;
            packet->head.S = 0x26;
            packet->head.D = 0x00;

            packet->payload.dst_addr = 0x01;
            packet->payload.dst_addr = packet->payload.dst_addr << 8;
            packet->payload.src_addr = 0x00;
            packet->payload.src_addr = packet->payload.src_addr << 8;
//            packet->payload.msg_len = 0x0E;
            packet->payload.msg_len = 0x12;
            packet->payload.groupID = 0x22;
            packet->payload.handlerID = 0x03;

            packet->payload.content.msgType = type;
            memcpy(&(packet->payload.content.msgName), interest, sizeof(interest_name));

            //packet->payload.content.data[0] = 0x01;
            //packet->payload.content.data[2] = 0xff;
            //packet->payload.content.data[3] = 0xff;

            packet->end.CRC = crccal((unsigned char *) packet + 1, sizeof(tinyosndw_packet) - 4);

            packet->end.F = 0x7e;
            printhex_macaddr((char *) packet, sizeof(tinyosndw_packet), " ");
            uint8_t loopflag = 1;
            uint8_t ARQ = 0;
            //\u5f00\u542f\u5931\u8d25\u91cd\u4f20\u673a\u5236\uff0c\u6700\u591a5\u6b21
            while (loopflag) {
                //\u901a\u8fc7\u4e32\u53e3\u53d1\u9001Interest
                nread = write(fdusb, (char *) packet, sizeof(tinyosndw_packet));
                if (nread == -1) {
                    if (ARQ < 5) {
                        printf("write failed!!\n");
                        ARQ++;
                    }
                    else {
                        printf("failed to write the interest into the USB port!\n");
                        loopflag = 0;
                    }
                }
                else {
                    loopflag = 0;
                    DEBUG printf("write usb successed!!-----NO:[%d]\n", g_count++);
                }
            }
            free(packet);
        }


        int serial_manager::set_opt(int nSpeed, int nBits, char nEvent, int nStop) {
            struct termios newtio, oldtio;
            if (tcgetattr(fdusb, &oldtio) != 0) {
                perror("SetupSerial 1");
                return -1;
            }
            bzero(&newtio, sizeof(newtio));
            newtio.c_cflag |= CLOCAL | CREAD;
            newtio.c_cflag &= ~CSIZE;

            switch (nBits) {
                case 7:
                    newtio.c_cflag |= CS7;
                    break;
                case 8:
                    newtio.c_cflag |= CS8;
                    break;
            }

            switch (nEvent) {
                case 'O':
                    newtio.c_cflag |= PARENB;
                    newtio.c_cflag |= PARODD;
                    newtio.c_iflag |= (INPCK | ISTRIP);
                    break;
                case 'E':
                    newtio.c_iflag |= (INPCK | ISTRIP);
                    newtio.c_cflag |= PARENB;
                    newtio.c_cflag &= ~PARODD;
                    break;
                case 'N':
                    newtio.c_cflag &= ~PARENB;
                    break;
            }

            switch (nSpeed) {
                case 2400:
                    cfsetispeed(&newtio, B2400);
                    cfsetospeed(&newtio, B2400);
                    break;
                case 4800:
                    cfsetispeed(&newtio, B4800);
                    cfsetospeed(&newtio, B4800);
                    break;
                case 9600:
                    cfsetispeed(&newtio, B9600);
                    cfsetospeed(&newtio, B9600);
                    break;
                case 115200:
                    cfsetispeed(&newtio, B115200);
                    cfsetospeed(&newtio, B115200);
                    break;
                case 460800:
                    cfsetispeed(&newtio, B460800);
                    cfsetospeed(&newtio, B460800);
                    break;
                default:
                    cfsetispeed(&newtio, B9600);
                    cfsetospeed(&newtio, B9600);
                    break;
            }
            if (nStop == 1)
                newtio.c_cflag &= ~CSTOPB;
            else if (nStop == 2)
                newtio.c_cflag |= CSTOPB;
            newtio.c_cc[VTIME] = 0;//\u91cd\u8981
            newtio.c_cc[VMIN] = 1;//\u8fd4\u56de\u7684\u6700\u5c0f\u503c  \u91cd\u8981
            tcflush(fdusb, TCIFLUSH);
            if ((tcsetattr(fdusb, TCSANOW, &newtio)) != 0) {
                perror("com set error");
                return -1;
            }
            //  //printf("set done!\n\r");
            return 0;
        }

        void serial_manager::dfs_topo_tree(BTNode *node) {
            if (node->child_num == 0) {
                free(node);
                return;
            }
            else {
                int i;
                for (i = 0; i < node->child_num; i++) {
                    dfs_topo_tree(node->ptr[i]);
                }
                free(node);
            }
        }

        void serial_manager::topo_tree_recycling() {
            if (topo_head->child_num == 0)
                return;
            dfs_topo_tree(topo_head->ptr[0]);
            topo_head->child_num = 0;
            topo_head->ptr[0] = NULL;
            return;
        }

        void serial_manager::topo_tree_build(BTNode **node, uint16_t *node_list, int index,
                                             uint8_t *topo_rebuild_flag) {
            if (index < 0)
                return;
            int i;
            for (i = 0; i < (*node)->child_num; i++) {
                if ((*node)->ptr[i]->nodeID == node_list[index]) {
                    topo_tree_build(&((*node)->ptr[i]), node_list, index - 1, topo_rebuild_flag);
                    break;
                }
            }
            if (i == (*node)->child_num) {

                if (topo_tree_node_check[node_list[index]] == 1) {

                    *topo_rebuild_flag = 1;
                    return;
                }
                (*node)->ptr[i] = (BTNode *) malloc(sizeof(BTNode));
                (*node)->ptr[i]->nodeID = node_list[index];
                (*node)->ptr[i]->parent = (*node);
                (*node)->ptr[i]->child_num = 0;
                (*node)->child_num++;
                topo_tree_node_check[node_list[index]] = 1;
                topo_tree_build(&((*node)->ptr[i]), node_list, index - 1, topo_rebuild_flag);
            }
        }

        void serial_manager::topo_tree_level(BTNode **bt_buf, int n) {
            if (n == 0) return;
            int i, j, k = 0;
            BTNode *tmp_buf[PTR_MAX];
            for (i = 0; i < n; i++) {
                printf("%d ", bt_buf[i]->nodeID);
                for (j = 0; j < bt_buf[i]->child_num; j++)
                    tmp_buf[k++] = bt_buf[i]->ptr[j];
            }
            printf("\n");
            topo_tree_level(tmp_buf, k);
        }

        void serial_manager::topo_tree_show(BTNode *head) {
            printf("show the topo tree!~\n");
            if (head == NULL) {
                printf("the head is null!~\n");
                return;
            }
            BTNode *bt_buf[PTR_MAX];
            bt_buf[0] = head;
            //\u5c42\u6b21\u904d\u5386topo\u8282\u70b9
            topo_tree_level(bt_buf, 1);
        }

        /**
         * \u62d3\u6251\u7ba1\u7406\u4e0e\u91cd\u5efa
         */
        void serial_manager::topo_management(std::set<std::string> & topo_dataset ) {
            DEBUG printf("\u3010create topo management thread successed!!\u3011\n");
            uint8_t topo_rebuild_flag = 0;
//			char topo_tmp[64]={'\0'};
			std::string topo_tmp;
			std::ostringstream topo_data;
			
            while (thread_flag) {
                //\u7b49\u5f85\u91cd\u5efa\u4fe1\u53f7\u5230\u6765
                sem_wait(&sem_queue);
                printf("\u5f00\u59cb\u6253\u5370\u62d3\u6251\u4fe1\u606f\n");
                topo_msg *topo = queue[bottom++];
                int i;
                if (bottom >= PTR_MAX)
                    bottom = bottom % PTR_MAX;
				
                for (i = 0; i < topo->num; i++) {
                    //\u6253\u5370\u62d3\u6251\u7ed3\u6784
                    char tmp[64];
                    if(topo->data[i] == 1){
//						printf("%d ", topo->data[i]);
						sprintf(tmp,"%d",topo->data[i]);
                    }
					else{
//                    	printf("%d -> ", topo->data[i]);
						sprintf(tmp,"%d",topo->data[i]);
					}
					
					if(i>=1){
						topo_data<<topo_tmp<<"->"<<tmp;
						std::cout<<topo_data.str()<<std::endl;
						topo_dataset.insert(topo_data.str());
						topo_data.str("");
					}
					topo_tmp=tmp;
//					topo_data+=topo_tmp;
					
                }
	           
//				std::cout<<topo_tmp<<std::endl;
//				topo_dataset.insert(topo_tmp);
//				topo_tmp.clear();
//				printf("%s\n",topo_tmp.data());
//                topo_tree_recycling();
                topo_tree_build(&topo_head, topo->data, topo->num - 1, &topo_rebuild_flag);
                if (topo_rebuild_flag) {
                    memset(topo_tree_node_check, 0, 256);
                    topo_tree_recycling();
                    topo_rebuild_flag = 0;
                }
                topo_tree_show(topo_head);
                free(topo);
            }
            return;
        }

        void serial_manager::sync_time(int local_timestamp) {
            interest_name *name = (interest_name *) malloc(sizeof(interest_name));
            memset(name, 0, sizeof(interest_name));
            name->time_period.start_time = local_timestamp;
            name->time_period.end_time = local_timestamp;
			std::time(&globe_timestamp);
            //\u5c01\u88c5tinyOS\u6570\u636e\u5305\uff0c\u901a\u8fc7\u4e32\u53e3\u5c06Interest\u53d1\u9001\u7ed9\u4e0b\u6e38\u4f20\u611f\u5668
            usb_write(name, TIME);
            free(name);
        }

        void serial_manager::printhex_macaddr(void *hex, int len, const char *tag) {
            int i;
            unsigned char *p = (unsigned char *) hex;

            if (len < 1)
                return;

            for (i = 0; i < len - 1; i++) {
                if (*p < 0x10)
                    printf("0%x%s", *p++, tag);
                else
                    printf("%2x%s", *p++, tag);
            }

            if (*p < 0x10)
                printf("0%x", *p++);
            else
                printf("%2x", *p++);
        }

        int serial_manager::pack_data_content(char *name, char *content) {
            // TODO
            return 0;
        }
    }
}

