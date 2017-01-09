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
#include "fw/forwarder.hpp"
#include "fw/location-route-strategy.hpp"
#include "nfd.hpp"
#include "../../ndn-cxx-master/src/interest-filter.hpp"
#include "../../ndn-cxx-master/src/encoding/nfd-constants.hpp"
#include "../../../../../usr/include/c++/4.8/stdexcept"
#include <ndn-cxx/management/nfd-controller.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/face-uri.hpp>
#include <boost/regex.hpp>
#include <ndn-cxx/management/nfd-face-query-filter.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <memory>
#include <ndn-cxx/util/time.hpp>
#include <climits>
#include <ctime>
#include <queue>



namespace nfd{
    namespace fw{
        class  LocationRouteStrategy;
    }
	class Forwarder;
	class Nfd;
}

namespace nfd{
	namespace gateway{

//	using namespace ndn;
	using ndn::InterestFilter;
	using ndn::Interest; 
	using ndn::Name;
	using namespace ndn::nfd;
	
	
	class serial_manager;
	
	class Nwd{
	public:
		Nwd(nfd::Nfd &);
		
		void
 		initialize();

		class Error : public std::runtime_error
 	    {
  			public:
   		   		 explicit
   				 Error(const std::string& what)
      				: std::runtime_error(what)
    			{
    			}
 		 };

		class FaceIdFetcher
		{
			
			public:
				typedef std::function<void(uint32_t)> SuccessCallback;
   				typedef std::function<void(const std::string&)> FailureCallback;

				static void
    			start(ndn::Face& face, ndn::nfd::Controller& controller,const std::string& input,bool allowCreate,
          			const SuccessCallback& onSucceed,const FailureCallback& onFail);
				
			private:
				FaceIdFetcher(ndn::Face& face,ndn::nfd::Controller& controller,bool allowCreate,
                  const SuccessCallback& onSucceed,const FailureCallback& onFail);
				
				void
    			startGetFaceId(const ndn::util::FaceUri& faceUri);

				void
    			onCanonizeSuccess(const ndn::util::FaceUri& canonicalUri);

				void
  			    onCanonizeFailure(const std::string& reason);

				void
   			    onQuerySuccess(const ndn::ConstBufferPtr& data,const ndn::util::FaceUri& canonicalUri);

				void
    			onQueryFailure(uint32_t errorCode,const ndn::util::FaceUri& canonicalUri);

				void
   			    fail(const std::string& reason);

				void
    			succeed(uint32_t faceId);

				void
   			    startFaceCreate(const ndn::util::FaceUri& canonicalUri);

				void
    			onFaceCreateError(uint32_t code,const std::string& error,const std::string& message);
				
    			ndn::Face& m_face;
    			ndn::nfd::Controller& m_controller;
    			bool m_allowCreate;
    			SuccessCallback m_onSucceed;
    			FailureCallback m_onFail;
    			ndn::ValidatorNull m_validator;
		};
	public:
		static const ndn::time::milliseconds DEFAULT_EXPIRATION_PERIOD;
		static const uint64_t DEFAULT_COST;
		uint64_t m_flags;
 		uint64_t m_cost;
  		uint64_t m_faceId;
   		uint64_t m_origin;
		ndn::time::milliseconds m_expires;
	    FacePersistency m_facePersistency;

		
	private:
		void
  		onInterest(const InterestFilter& filter, const Interest& interest);

		void
		Topo_onInterest(const InterestFilter& filter, const Interest& interest);

		void
		Location_onInterest(const InterestFilter& filter, const Interest& interest);

		void
		Wifi_Register_onInterest(const InterestFilter& filter, const Interest& interest);

        void
        wifi_update_id_location(std::string& str);

        void
        Wifi_Location_onInterest(const InterestFilter& filter, const Interest& interest);

        void
        Wifi_Topo_onInterest(const InterestFilter& filter, const Interest& interest);

		void
  		onRegisterFailed(const Name& prefix, const std::string& reason);

		void
        Wsn_Range_onInterest(const InterestFilter& filter, const Interest& interest);

		void 
		listen_wsn_data(serial_manager *sm);

		void 
		manage_wsn_topo(serial_manager *sm);

		void
		send_data(std::string ,std::string );

	    void 
		wait_data();

		void
		time_sync_init();
		
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

		void
  		ribRegisterPrefix();

		void
		strategyChoiceSet(std::string name,std::string strategy);

		void
  		onSuccess(const ControlParameters& commandSuccessResult,const std::string& message);

		void
 	    onError(uint32_t code, const std::string& error, const std::string& message);

		void
  		onObtainFaceIdFailure(const std::string& message);
		
		ndn::Face m_face;
		ndn::KeyChain m_keyChain;
		ndn::nfd::Controller m_controller;
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
		time_t globe_timestamp;
		std::set<WsnData> data_set;
		std::set<std::string> topo_data;
		std::map<std::string,std::string> wsn_location_map;
        std::map<std::string,std::string> wifi_location_map;
		std::shared_ptr<Forwarder> m_forwarder;
		std::string remote_name;
		std::string face_name;
		int wsn_nodes;
		bool handle_interest_busy;

		std::queue<std::string> receive_in_queue;
		
	};

	}
}
#endif
