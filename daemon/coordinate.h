//
// Created by youngwb on 3/7/17.
//

#ifndef NFD_MASTER_COORDINATE_H
#define NFD_MASTER_COORDINATE_H

#include <boost/functional/hash.hpp>

namespace nfd {
    namespace gateway {

class Coordinate
{
private:
    double longitude;  //经度
    double latitude;  //纬度
public:
    Coordinate(double x,double y):longitude(x),latitude(y){}
    double get_longitude() const
    {
        return  longitude;
    }

    double  get_latitude() const
    {
        return  latitude;
    }
    friend class  CoordinateHash;
    friend class  CoordinateEqual;
//    bool operator == (const Coordinate& c)
//    {
//        return longitude == c.longitude && latitude == c.latitude;
//    }

};

class CoordinateHash
{
public:
    std::size_t operator() (const Coordinate& c) const
    {
        return boost::hash_value (std::pair<double,double>(c.latitude,c.longitude));
    }

};

class CoordinateEqual
{
public:
    bool operator() (const Coordinate& c1,const Coordinate& c2) const
    {
        if(c1.latitude == c2.latitude&& c1.longitude == c2.longitude)
            return  true;
        else
            return  false;
    }
};

double distance(const Coordinate& c1, const Coordinate& c2);
std::ostream& operator << (std::ostream& output,const Coordinate& c);


    }
}


#endif //NFD_MASTER_COORDINATE_H
