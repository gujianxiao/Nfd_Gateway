#ifndef NFD_GATEWAY_NWD_HPP
#define NFD_GATEWAY_NWD_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/transport/transport.hpp>
#include <boost/thread.hpp>
#include "mgmt/serial_manager.hpp"
#include <boost/thread/lock_factories.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include "core/global-io.hpp"
#include <boost/chrono.hpp>
#include <boost/bind.hpp>
#include "wsn-data.hpp"

namespace nfd{
	namespace gateway{

//	using namespace ndn;
	using ndn::InterestFilter;
	using ndn::Interest; 
	using ndn::Name;

	
	
	class serial_manager;
	
	class Nwd{
	public:
		Nwd();
		
		void
 		initialize();
	private:
		void
  		onInterest(const InterestFilter& filter, const Interest& interest);

		void
		Topo_onInterest(const InterestFilter& filter, const Interest& interest);

		void
		Location_onInterest(const InterestFilter& filter, const Interest& interest);
	

		void
  		onRegisterFailed(const Name& prefix, const std::string& reason);

		void 
		listen_wsn_data(serial_manager *sm);

		void 
		manage_wsn_topo(serial_manager *sm);

		void
		send_data(std::string ,std::string );

	    void 
		wait_data();

		void
		time_sync();

		void 
  		search_dataset(std::string In_Name);

		bool 
  		search_dataset_position(std::string Interest,WsnData dataval);

		bool
  		search_dataset_time(std::string In_Name,WsnData dataval);

		bool 
  		search_dataset_type(std::string In_Name,WsnData dataval);
		
		ndn::Face m_face;
		ndn::KeyChain m_keyChain;
		serial_manager m_serialManager;
		boost::thread_group threadGroup;
		std::map<std::string,std::set<std::string>> interest_list;
		boost::mutex mu;
		boost::condition_variable_any data_ready;
		boost::asio::io_service io; 
		
//		boost::asio::io_service::work m_work;
		boost::asio::steady_timer m_t;
		boost::asio::steady_timer m_tsync; //for time sync
		int local_timestamp;
		std::set<WsnData> data_set;
		std::set<std::string> topo_data;
		std::map<std::string,std::string> location_map;
	};

	}
}
#endif