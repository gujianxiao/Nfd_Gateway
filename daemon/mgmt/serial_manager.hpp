//
// Created by xiayula on 16-5-31.
//

#ifndef NDN_CXX_SERIAL_MANAGER_H
#define NDN_CXX_SERIAL_MANAGER_H

#include <inttypes.h>
#include <stdlib.h>
#include <semaphore.h>
#include <boost/thread.hpp>
#include <boost/thread/lock_factories.hpp>
#include <ctime>


//#define SHORTEST_DATA_FRAME_LEN 25
#define SHORTEST_DATA_FRAME_LEN 25
#define USB_PATH_PORT   "/dev/ttyUSB0"
#define MY_NOTE             1//\u662f\u5426\u6253\u5370\u8c03\u8bd5\u4fe1\u606f\uff1a0-\u4e0d\u6253\u5370; 1-\u6253\u5370
#define DEBUG                 if(MY_NOTE)
#define NAME_PREFIX             "wsn"

#define PTR_MAX 128
#define TOPO_MSG_LEN 20


namespace nfd {
    namespace gateway {
#pragma pack(1)
        typedef struct BTNode{
            int child_num;
            struct BTNode *ptr[PTR_MAX];
            uint16_t nodeID;
            struct BTNode *parent;
        }BTNode;

        typedef enum Type {
            Light,
            Temp,
            Humidity,
        } Type;
        typedef enum messageType {
            TIME = 0,
            BRO = 1,
            IN = 2,             //interest
            DATA = 3,       //content
            MAPPING = 4,     //topology responding type
            TOPOLOGY = 5,     //topology asking type
        } messageType;

        /**
        * \u7ecf\u7eac\u5ea6\u7ed3\u6784
        */
        typedef struct point {
            uint16_t x; //\u7ecf\u5ea6
            uint16_t y; //\u7ef4\u5ea6
        } point;

        /**
         * \u8303\u56f4\u7ed3\u6784\uff0c\u7528\u6765\u5f53\u4f5c\u8def\u7531\u80fd\u529b\uff0c\u8fdb\u884c\u524d\u7f00\u5339\u914d
         * \u8303\u56f4\u7528\u77e9\u5f62\u8868\u793a
         */
        typedef struct location {
            point leftUp; //\u77e9\u5f62\u5de6\u4e0a\u89d2\u8282\u70b9
            point rightDown; //\u77e9\u5f62\u53f3\u4e0b\u89d2\u8282\u70b9
        } location;

        typedef struct period{
            uint16_t start_time;
            uint16_t end_time;
        } period; //4B


        /**
         * name\u7684\u7ed3\u6784
         * dataType:
         * Temp,Light,Energy
         */
        typedef struct name {
            location ability;
            period time_period;
            uint16_t dataType;
        } interest_name;//10B --> 14B

        typedef struct message {
            uint16_t msgType;
            interest_name msgName;
            uint16_t data;
        } Msg; //14B --> 18B

        typedef struct topology_message {
            uint16_t num;
            uint16_t data[10];
        } topo_msg;


        typedef struct node_mapping {
            short expire;
            point coordinate;
        } node_mapping;

        typedef struct node_info {
            location coordinate;
            uint16_t nodeID;
        } node_info;


        typedef struct tinyosndw_payload {
            uint16_t dst_addr;
            uint16_t src_addr;
            uint8_t msg_len;
            uint8_t groupID;
            uint8_t handlerID;
            Msg content;
        } tinyosndw_payload;//21B --> 25B

        typedef struct tinyosndw_head {
            uint8_t F;
            uint8_t P;
            uint8_t S;
            uint8_t D;
        } tinyosndw_head;//4B

        typedef struct tinyosndw_end {
            uint16_t CRC;
            uint8_t F;
        } tinyosndw_end;//3B

        typedef struct tinyosndw_packet {
            tinyosndw_head head;
            tinyosndw_payload payload;
            tinyosndw_end end;
        } tinyosndw_packet;
#pragma pack()

        class serial_manager {
        public:
            uint16_t local_timestamp;

            serial_manager(boost::condition_variable_any& );

            void read_data(std::map<std::string,std::set<std::string>>& ,std::map<std::string,std::string>&);

            int set_opt(int nSpeed, int nBits, char nEvent, int nStop);

            /**
             * Print the MAC addr in HEX
             */
            void printhex_macaddr(void *hex, int len, const char *tag);

            int pack_data_content(char* name, char *content);

            void topo_management(std::set<std::string> &);

            int handle_interest(char *interest);

            /**
            * write the interest into USB
            */
            void usb_write(interest_name *interest, uint16_t type);

            uint16_t crcByte(uint16_t crc, uint8_t b);

            uint16_t crccal(unsigned char* buf, int len);
            void topo_tree_traversal(BTNode **node, int *count, char *buf, char *temp);

            void sync_time(int);

			bool position_belong_interest(std::string Interest,int point_x,int point_y);
			bool time_belong_interest(std::string Interest,int datatime);
			bool datatype_equal_interest(std::string Interest,std::string datatype);
        private:
            int fdusb;
            int datagotflag;
            int total_read;
            int nread;
            char *start_p;
            Msg *recv_data;
            topo_msg *queue[PTR_MAX]; //\u62d3\u6251\u73af\u5f62\u961f\u5217
            uint16_t top;
            uint16_t bottom;
            node_mapping nodeID_mapping_table[256];
            int move_pos;
            sem_t sem_queue;
            sem_t sem_interest;
            int data_flag;
            BTNode *topo_head;
            uint8_t topo_tree_node_check[256];
            int thread_flag;
            void topo_tree_show(BTNode *head);
            void topo_tree_level(BTNode **bt_buf, int n);
            void topo_tree_build(BTNode **node, uint16_t *node_list, int index, uint8_t *topo_rebuild_flag);
            void topo_tree_recycling();
            void dfs_topo_tree(BTNode *node);

            char *scope_name;

//			boost::mutex& mu;
			boost::condition_variable_any& data_ready;

            int g_count;

			time_t globe_timestamp;
        };
    }
}
#endif //NDN_CXX_SERIAL_MANAGER_H

