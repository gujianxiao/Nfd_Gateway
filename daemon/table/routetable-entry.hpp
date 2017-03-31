//
// Created by youngwb on 3/25/17.
//

#ifndef NFD_MASTER_ROUTETABLE_ENTRY_HPP
#define NFD_MASTER_ROUTETABLE_ENTRY_HPP

#include "../coordinate.h"
namespace nfd{
    namespace gateway{
    class RouteTableEntry
    {
    public:
        enum Reach_Status {neighbor,unreachable,reachable,unknown,minlocal};
        enum Send_Status {flood,sending,notsending};
    private:
//        gateway::Coordinate dest_;
        gateway::Coordinate nexthop_;
        double weight_;
        Reach_Status rs_flag_;
        Send_Status ss_flag_;
    public:

        RouteTableEntry(const Coordinate &nexthop, double weight, Reach_Status rs_flag,Send_Status ss_flag):
            nexthop_(nexthop),weight_(weight),rs_flag_(rs_flag),ss_flag_(ss_flag){}

        RouteTableEntry():nexthop_(),weight_(0),rs_flag_(unknown),ss_flag_(notsending){}

//        gateway::Coordinate get_dest() const
//        {
//            return  dest_;
//        }


        gateway::Coordinate get_nexthop() const
        {
            return nexthop_;
        }

        double get_weight() const
        {
            return weight_;
        }

        Reach_Status  get_reachstatus() const
        {
            return rs_flag_;
        }

        Send_Status get_sendstatus() const
        {
            return ss_flag_;
        }

        void set_reachstatus(Reach_Status s)
        {
            rs_flag_=s;
        }

        void set_sendstatus(Send_Status s)
        {
            ss_flag_=s;
        }
    };

    std::ostream& operator<< (std::ostream& os ,  const RouteTableEntry & re);
    }
}
#endif //NFD_MASTER_ROUTETABLE_ENTRY_HPP
