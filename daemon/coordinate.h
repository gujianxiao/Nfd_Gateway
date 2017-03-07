//
// Created by youngwb on 3/7/17.
//

#ifndef NFD_MASTER_COORDINATE_H
#define NFD_MASTER_COORDINATE_H

#include <boost/variant/variant.hpp>

namespace nfd {
    namespace gateway {
class Coordinate
{
private:
    double longitude;  //经度
    double latitude;  //纬度
public:
    double get_longgitude() const
    {
        return  longitude;
    }

    double  get_latitude() const
    {
        return  latitude;
    }


};



class CoordinateHash
{
public:
    std::size_t operator() (const Coordinate& c) const
    {
        return boost::hash_value (std::pair<double,double>(c.get_latitude(),c.get_longgitude()));
    }

};

class CoordinateEqual
{
public:
    bool operator() (const Coordinate& c1,const Coordinate& c2)
    {
        if(c1.get_latitude() == c2.get_latitude() && c1.get_longgitude() == c2.get_longgitude())
            return  true;
        else
            return  false;
    }

};



    }
}


#endif //NFD_MASTER_COORDINATE_H
