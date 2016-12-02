#include "nwd.hpp"
#include "wsn-data.hpp"

namespace nfd {
	namespace gateway{

	const ndn::time::milliseconds Nwd::DEFAULT_EXPIRATION_PERIOD = ndn::time::milliseconds::max();
	const uint64_t Nwd::DEFAULT_COST = 0;
		
	Nwd::Nwd(nfd::Nfd& nfd) : m_flags(ROUTE_FLAG_CHILD_INHERIT), m_cost(DEFAULT_COST) , 
		m_origin(ROUTE_ORIGIN_STATIC),m_expires(DEFAULT_EXPIRATION_PERIOD),
		m_facePersistency(ndn::nfd::FacePersistency::FACE_PERSISTENCY_PERSISTENT),
		m_controller(m_face, m_keyChain),m_serialManager(data_ready),m_t(m_face.getIoService()),
		m_tsync(m_face.getIoService()),local_timestamp(0),globe_timestamp(0),m_forwarder(nfd.get_Forwarder()),wsn_nodes(0),
		handle_interest_busy(false)
	{
//		io.run();
	}
	
	void
	Nwd::initialize()
	{
		
		m_face.setInterestFilter("/wsn/topo",
                             bind(&Nwd::Topo_onInterest, this, _1, _2),
                             ndn::RegisterPrefixSuccessCallback(),
                             bind(&Nwd::onRegisterFailed, this, _1, _2));

		
		m_face.setInterestFilter("/wsn",
									 bind(&Nwd::onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));

		m_face.setInterestFilter("/wsn/location",
									 bind(&Nwd::Location_onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));

		m_face.setInterestFilter("/wifi/register",
									 bind(&Nwd::Wifi_Register_onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));
		
//		m_face.setInterestFilter("/wifi/",
//									 bind(&Nwd::Wifi_onInterest, this, _1, _2),
//									 ndn::RegisterPrefixSuccessCallback(),
//									 bind(&Nwd::onRegisterFailed, this, _1, _2));

		

		m_face.setInterestFilter("/localhost/wsn/range",
									 bind(&Nwd::Wsn_Range_onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));
		

		threadGroup.create_thread(boost::bind(&Nwd::listen_wsn_data, this, &m_serialManager));
		threadGroup.create_thread(boost::bind(&Nwd::wait_data,this));
		threadGroup.create_thread(boost::bind(&Nwd::time_sync_init,this));

//		std::cout<<"data_set max_size is"<<data_set.max_size()<<std::endl;
		
//		threadGroup.create_thread(boost::bind(&Nwd::io_service_run,this));
        threadGroup.create_thread(boost::bind(&Nwd::manage_wsn_topo, this, &m_serialManager));
    	m_face.processEvents();
		
	}

//	void
//	Nwd::Wifi_onInterest(const InterestFilter& filter, const Interest& interest)
//	{
//		std::cout << "<< I: " << interest << std::endl;
//		std::string interest_name = interest.getName().toUri();
//		
//	}

	void
	Nwd::Wsn_Range_onInterest(const InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		int max_x=0,max_y=0,min_x=INT_MAX,min_y=INT_MAX;
		for(auto& itr:location_map){
//			std::cout<<itr.second<<std::endl;
			std::string::size_type x_start=itr.second.find("(");
			std::string::size_type x_end=itr.second.find(",");
			std::string::size_type y_start=x_end;
			std::string::size_type y_end= itr.second.find(")");
			std::string x=itr.second.substr(x_start+1,x_end-x_start-1);
			std::string y=itr.second.substr(y_start+1,y_end-y_start-1);
			std::cout<<x<<","<<y<<std::endl;
			int x_int=std::stoi(x);
			int y_int=std::stoi(y);
			if(x_int<min_x)
				min_x=x_int;
			if(x_int>max_x)
				max_x=x_int;
			if(y_int<min_y)
				min_y=y_int;
			if(y_int>max_y)
				max_y=y_int;
		
		}
		wsn_nodes=location_map.size();
		Name dataName(interest_name);
	  	std::string data_val("("+std::to_string(min_x)+","+std::to_string(max_y)+")/("+
			std::to_string(max_x)+","+std::to_string(min_y)+") "+to_string(wsn_nodes)+"$");
	  	shared_ptr<Data> data = make_shared<Data>();
     	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
	  	m_keyChain.sign(*data);
	
	    std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);	
	}

	void
	Nwd::Wifi_Register_onInterest(const InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();

		face_name=interest_name;
		face_name.erase(5,9);
		std::cout<<face_name<<std::endl;
		wifi_user_id.push_back(face_name);
		auto pit_entry_itr=m_forwarder->getPit().begin();
//		++pit_entry_itr;
		for(;pit_entry_itr!=m_forwarder->getPit().end();pit_entry_itr++){
			
			std::ostringstream os;
			os<<(*pit_entry_itr).getInterest()<<std::endl;
			if(os.str().find("/wifi/register")!=std::string::npos){
//				std::cout<<os.str()<<std::endl;
				for(auto pit_entryIn_itr=(*pit_entry_itr).in_begin();pit_entryIn_itr!=(*pit_entry_itr).in_end();pit_entryIn_itr++){
//					std::cout<<(*pit_entryIn_itr).getInterest()<<std::endl;
					std::ostringstream oss;
//					std::cout<<(*pit_entryIn_itr).getFace()->getLocalUri()<<std::endl;
					oss<<(*pit_entryIn_itr).getFace()->getRemoteUri()<<std::endl;
					remote_name=oss.str();
					std::string::size_type pos=remote_name.rfind(":");
					remote_name.erase(pos);
					std::cout<<remote_name<<std::endl;
//					std::cout<<"face name:"<<face_name<<std::endl;
//					ribRegisterPrefix();
					
					
				}
			}
		}
//		boost::this_thread::sleep(boost::posix_time::seconds(2)); 
		

		Name dataName(interest_name);
	  	std::string data_val("success");
	  	shared_ptr<Data> data = make_shared<Data>();
     	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
	  	m_keyChain.sign(*data);
	
	    std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);

		face_name="/wifi";
		std::cout<<"face name:"<<face_name<<std::endl;
		ribRegisterPrefix();
        strategyChoiceSet(face_name,"ndn:/localhost/nfd/strategy/broadcast");

//		nfd::FaceTable& faceTable = m_forwarder->getFaceTable();
//		for(auto itr=faceTable.begin();itr!=faceTable.end();itr++){
//			std::cout<<(*itr)->getRemoteUri()<<std::endl;
//		}
//		for(auto itr:faceTable.m_faces){
//			std::cout<<*itr.first<<std::endl;
//		}
//		m_forwarder->getFaceTable();

	}

    void
    Nwd::strategyChoiceSet(std::string name,std::string strategy)
    {


        ControlParameters parameters;
        parameters
                .setName(name)
                .setStrategy(strategy);

        m_controller.start<StrategyChoiceSetCommand>(parameters,
                                                     bind(&Nwd::onSuccess, this, _1,
                                                          "Successfully set strategy choice"),
                                                     bind(&Nwd::onError, this, _1, _2,
                                                          "Failed to set strategy choice"));
        std::cout<<"strategyset: "<<name<<" "<<strategy<<std::endl;
    }



	void
	Nwd::ribRegisterPrefix()
	{
//		m_name=face_name;
		const std::string& faceName=remote_name;
		FaceIdFetcher::start(m_face, m_controller, faceName, true,
                       [this] (const uint32_t faceId) {
                         ControlParameters parameters;
                         parameters
                           .setName(face_name)
                           .setCost(m_cost)
                           .setFlags(m_flags)
                           .setOrigin(m_origin)
                           .setFaceId(faceId);

                         if (m_expires != DEFAULT_EXPIRATION_PERIOD)
                           parameters.setExpirationPeriod(m_expires);

                         m_controller
                           .start<RibRegisterCommand>(parameters,
                                                      bind(&Nwd::onSuccess, this, _1,
                                                           "Successful in name registration"),
                                                      bind(&Nwd::onError, this, _1, _2,
                                                           "Failed in name registration"));
                       },
                       bind(&Nwd::onObtainFaceIdFailure, this, _1));
	}

	void
	Nwd::time_sync_init(){
		m_tsync.expires_from_now(std::chrono::seconds(70));
		m_tsync.async_wait(boost::bind(&Nwd::time_sync,this));
	}

	void
	Nwd::time_sync(){
		if(local_timestamp % (3000) ==0){
			local_timestamp=0;
			m_serialManager.sync_time(local_timestamp);
			std::time(&globe_timestamp);
		}
		
//		
		++local_timestamp;
//		std::cout<<"time sync:"<<globe_timestamp+local_timestamp<<std::endl;

		m_tsync.expires_from_now(std::chrono::seconds(1));//set 1s timer
		m_tsync.async_wait(boost::bind(&Nwd::time_sync,this));
	}

	void
	Nwd::Location_onInterest(const InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		std::string data_val;
		for(auto& itr:location_map){
			data_val+=itr.first;
			data_val+="->";
			data_val+=itr.second;
			data_val+="$$";
		}
	    Name dataName(interest_name);
	  
	   	shared_ptr<Data> data = make_shared<Data>();
      	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
      	data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
	 	 m_keyChain.sign(*data);

	  	std::cout << ">> D: " << *data << std::endl;
      	m_face.put(*data);
		
	}

	void
	Nwd::Topo_onInterest(const InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		std::string data_val;	
//		std::cout<<"before"<<std::endl;
		for(auto & itr:topo_data){
			data_val+=itr;
			std::cout<<itr<<std::endl;
			data_val+="$$";
		}
		Name dataName(interest_name);
	  
	  shared_ptr<Data> data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(time::seconds(10));
      data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
	  m_keyChain.sign(*data);

	  std::cout << ">> D: " << *data << std::endl;
      m_face.put(*data);
		
	}	
	

	void
  	Nwd::onInterest(const InterestFilter& filter, const Interest& interest)
  	{
    	
		std::string interest_name = interest.getName().toUri();
		if(interest_name.find("topo") != std::string::npos)
			return ;
		if(interest_name.find("location") != std::string::npos)
			return ;
		if(interest_name.find("range") != std::string::npos)
			return ;
		std::cout << "<< I: " << interest << std::endl;
		
		std::string::size_type pos = 0;
        while ((pos = interest_name.find("%2C", pos)) != std::string::npos){
            interest_name.replace(pos, 3, ",");
            pos += 1;
        }
		interest_name.replace(0, 1, "");
//		std::string interest_copy=interest_name;
		interest_list.insert(std::make_pair(interest_name,std::set<std::string>()));
		
//		std::string::size_type time_end_pos=interest_name.rfind("/");
//		std::string::size_type time_mid_pos=interest_name.rfind("/",time_end_pos-1);
//		std::string::size_type time_start_pos=interest_name.rfind("/",time_mid_pos-1);
//		int endtime=std::stoi(interest_name.substr(time_mid_pos+1,time_end_pos-time_mid_pos-1));
//		int starttime=std::stoi(interest_name.substr(time_start_pos+1,time_mid_pos-time_start_pos-1));
//	
//		starttime-=globe_timestamp;
//		endtime-=globe_timestamp;
//		std::cout<<"start:"<<starttime<<std::endl;
//		std::cout<<"end:"<<endtime<<std::endl;
//		if(starttime>=0 && endtime>=0){
//			interest_name.replace(time_start_pos+1,time_end_pos-time_start_pos-1,std::to_string(starttime)+"/"+to_string(endtime));
//			interest_name.replace(time_mid_pos+1,time_end_pos-time_mid_pos-1,std::to_string(endtime));
//		}else
//			return;
		receive_in_queue.push(interest_name);
		std::cout<<interest_name<<std::endl;
		
		
		std::time(&globe_timestamp);
		std::cout<<"time:"<<globe_timestamp<<std::endl;
		if(m_serialManager.time_belong_interest(interest_name,globe_timestamp)){			
			if(handle_interest_busy==false){
				handle_interest_busy=true;
				while(!receive_in_queue.empty()){
					std::string recv_in=receive_in_queue.front();
				
					char tmp[1024];
        			strcpy(tmp, recv_in.c_str());
					m_serialManager.handle_interest(tmp);
					receive_in_queue.pop();
					boost::this_thread::sleep(boost::posix_time::milliseconds(500));
				}
				handle_interest_busy=false;
				m_t.expires_from_now(std::chrono::milliseconds(1600));//set 400ms timer
				
				m_t.async_wait(boost::bind(&Nwd::wait_data,this));
			}
		}else{
			search_dataset(interest_name);
		}

		
//		getGlobalIoService().run();
//		std::cout<<"after timer"<<std::endl;
  	}

  void 
  Nwd::search_dataset(std::string In_Name){
  		std::string data_ret;
		for(auto &itr:data_set){
			if(search_dataset_position(In_Name,itr)){
				if(search_dataset_time(In_Name,itr)){
					if(search_dataset_type(In_Name,itr)){
//						std::cout<<"data_set match"<<std::endl;
						data_ret+=itr.data;
						data_ret+="$$";
					}
						
				}
			}
		}
		send_data(In_Name,data_ret);
  }

  bool 
  Nwd::search_dataset_type(std::string In_Name,WsnData dataval){
	std::string data_type = In_Name.substr(In_Name.rfind('/')+1);
//	std::cout<<In_Name<<std::endl;
//	std::cout<<data_type<<std::endl;
	if(data_type == dataval.type){
//		std::cout<<"dataset type match"<<std::endl;
		return true;
	}
	return false;
  }
	
  bool
  Nwd::search_dataset_time(std::string In_Name,WsnData dataval){
	std::string::size_type in_time_beg,in_time_mid,in_time_end;
	in_time_end=In_Name.rfind('/');
	in_time_mid=In_Name.rfind('/',in_time_end-1);
	in_time_beg=In_Name.rfind('/',in_time_mid-1);
	int In_Start_Time=std::stoi(In_Name.substr(in_time_beg+1,in_time_mid-in_time_beg-1));
	int In_End_Time=std::stoi(In_Name.substr(in_time_mid+1,in_time_end-in_time_mid-1));
//	std::cout<<"start:"<<In_Start_Time<<"end:"<<In_End_Time<<std::endl;
//	std::cout<<dataval.wsn_time<<std::endl;
	if(dataval.wsn_time>=In_Start_Time && dataval.wsn_time<=In_End_Time){
//		std::cout<<"dataset time match"<<std::endl;
		return true;
	}
	return false;
  }

  bool 
  Nwd::search_dataset_position(std::string Interest,WsnData dataval){
		std::string::size_type in_scope_beg,in_scope_mid,in_scope_end;
//		std::string::size_type data_scope_beg,data_scope_end;
		in_scope_beg=Interest.find('/');
		in_scope_mid=Interest.find('/',in_scope_beg+1);
		in_scope_end=Interest.find('/',in_scope_mid+1);
//		data_scope_beg=dataval.find('/');
//		data_scope_end=dataval.find('/',data_scope_beg+1);

		std::string in_scope_leftup=Interest.substr(in_scope_beg+1,in_scope_mid-in_scope_beg-1);
		std::string in_scope_rightdown=Interest.substr(in_scope_mid+1,in_scope_end-in_scope_mid-1);
		
		int in_leftup_x=std::stoi(in_scope_leftup.substr(0,in_scope_leftup.find(',')));
		int in_leftup_y=std::stoi(in_scope_leftup.substr(in_scope_leftup.find(',')+1));
		int in_rightdown_x=std::stoi(in_scope_rightdown.substr(0,in_scope_rightdown.find(',')));
		int in_rightdown_y=std::stoi(in_scope_rightdown.substr(in_scope_rightdown.find(',')+1));

//		std::string data_position=dataval.substr(data_scope_beg+1,data_scope_end-data_scope_beg-1);

//		int data_position_x=std::stoi(data_position.substr(0,data_position.find(',')));
//		int data_position_y=std::stoi(data_position.substr(data_position.find(',')+1));

//		std::cout<<"interest is "<<in_leftup_x<<" , "<<in_leftup_y<<"/"<<in_rightdown_x<<","<<in_rightdown_y<<std::endl;
//		std::cout<<"data is "<<data_position_x<<" , "<<data_position_y<<std::endl;
		if(dataval.position_x>=in_leftup_x && dataval.position_x<=in_rightdown_x){
			if(dataval.position_y>=in_rightdown_y && dataval.position_y<=in_leftup_y){			
//				std::cout<<"data_set position match"<<std::endl;
				return true;
			}	
		}
		return false;
  }

  void
  Nwd::onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cout << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

  void 
  Nwd::listen_wsn_data(serial_manager *sm)
  {    
  	   sm->read_data(interest_list,location_map);
	   
  }

  void 
  Nwd::wait_data(){
		std::cout<<"in wait data"<<std::endl;
		for(auto &itr:interest_list){
		    std::string data_ret;
		    std::string In_Name;
			In_Name=itr.first;
			for(auto &data:itr.second){
//				std::cout<<data<<std::endl;
//				data_ret+=data;
				data_set.insert(WsnData(data,local_timestamp+globe_timestamp)); //put in data_set
//				data_ret+="$$";
			}
			search_dataset(In_Name);
//			if(!data_ret.empty()){
//				
//				send_data(In_Name,data_ret);
//			}
		}
//		for(auto &itr:data_set)
//			std::cout<<itr<<std::endl;
		
  }
  
  void
  Nwd::send_data(std::string In_Name,std::string data_val)
  {
//  	  std::cout<<data_val<<std::endl;
	
	  Name dataName(In_Name);
	  
	  shared_ptr<Data> data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(time::seconds(10));
      data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
	  m_keyChain.sign(*data);

	  
		
	  std::cout << ">> D: " << *data << std::endl;
      m_face.put(*data);
	  interest_list.erase(In_Name); //delete the Interest stored in interest_list
  }

  void 
  Nwd::manage_wsn_topo(serial_manager *sm)
  {
	sm->topo_management(topo_data);
  }

  void
  Nwd::onSuccess(const ControlParameters& commandSuccessResult, const std::string& message)
  {
	 
	 std::cout << message << ": " << commandSuccessResult << std::endl;
  }

  void
  Nwd::onError(uint32_t code, const std::string& error, const std::string& message)
  {
     std::ostringstream os;
     os << message << ": " << error << " (code: " << code << ")";
     BOOST_THROW_EXCEPTION(Error(os.str()));
  }

  void
  Nwd::onObtainFaceIdFailure(const std::string& message)
 {
 	 BOOST_THROW_EXCEPTION(Error(message));
 }
  

  Nwd::FaceIdFetcher::FaceIdFetcher(ndn::Face& face,
                                   ndn::nfd::Controller& controller,
                                   bool allowCreate,
                                   const SuccessCallback& onSucceed,
                                   const FailureCallback& onFail)
  : m_face(face)
  , m_controller(controller)
  , m_allowCreate(allowCreate)
  , m_onSucceed(onSucceed)
  , m_onFail(onFail){}

 void
 Nwd::FaceIdFetcher::start(ndn::Face& face,
                           ndn::nfd::Controller& controller,
                           const std::string& input,
                           bool allowCreate,
                           const SuccessCallback& onSucceed,
                           const FailureCallback& onFail)
{
  // 1. Try parse input as FaceId, if input is FaceId, succeed with parsed FaceId
  // 2. Try parse input as FaceUri, if input is not FaceUri, fail
  // 3. Canonize faceUri
  // 4. If canonization fails, fail
  // 5. Query for face
  // 6. If query succeeds and finds a face, succeed with found FaceId
  // 7. Create face
  // 8. If face creation succeeds, succeed with created FaceId
  // 9. Fail
  
  boost::regex e("^[a-z0-9]+\\:.*");
  if (!boost::regex_match(input, e)) {
    try
      {
        u_int32_t faceId = boost::lexical_cast<uint32_t>(input);
		
        onSucceed(faceId);
        return;
      }
    catch (boost::bad_lexical_cast&)
      {
      	
        onFail("No valid faceId or faceUri is provided");
        return;
      }
  }
  else {
  	
    ndn::util::FaceUri faceUri;
    if (!faceUri.parse(input)) {
      onFail("FaceUri parse failed");
      return;
    }
	
    auto fetcher = new FaceIdFetcher(std::ref(face), std::ref(controller),
                                     allowCreate, onSucceed, onFail);

    fetcher->startGetFaceId(faceUri);
  }
}

void
Nwd::FaceIdFetcher::startGetFaceId(const ndn::util::FaceUri& faceUri)
{
 
  faceUri.canonize(bind(&FaceIdFetcher::onCanonizeSuccess, this, _1),
                   bind(&FaceIdFetcher::onCanonizeFailure, this, _1),
                   m_face.getIoService(), ndn::time::seconds(4));
}

void
Nwd::FaceIdFetcher::onCanonizeSuccess(const ndn::util::FaceUri& canonicalUri)
{
  
  ndn::Name queryName("/localhost/nfd/faces/query");
  ndn::nfd::FaceQueryFilter queryFilter;
  queryFilter.setRemoteUri(canonicalUri.toString());
  queryName.append(queryFilter.wireEncode());

  ndn::Interest interestPacket(queryName);
  interestPacket.setMustBeFresh(true);
  interestPacket.setInterestLifetime(ndn::time::milliseconds(4000));
  auto interest = std::make_shared<ndn::Interest>(interestPacket);
  
  ndn::util::SegmentFetcher::fetch(m_face, *interest,
                                   m_validator,
                                   bind(&FaceIdFetcher::onQuerySuccess,
                                        this, _1, canonicalUri),
                                   bind(&FaceIdFetcher::onQueryFailure,
                                        this, _1, canonicalUri));
} 

 void
 Nwd::FaceIdFetcher::onCanonizeFailure(const std::string& reason)
 {
	fail("Canonize faceUri failed : " + reason);
 }

 void
 Nwd::FaceIdFetcher::onQuerySuccess(const ndn::ConstBufferPtr& data,
                                    const ndn::util::FaceUri& canonicalUri)
{
  
  size_t offset = 0;
  bool isOk = false;
  ndn::Block block;
  std::tie(isOk, block) = ndn::Block::fromBuffer(data, offset);

  if (!isOk) {
    if (m_allowCreate) {
      startFaceCreate(canonicalUri);
    }
    else {
      fail("Fail to find faceId");
    }
  }
  else {
    try {
      FaceStatus status(block);
      succeed(status.getFaceId());
    }
    catch (const ndn::tlv::Error& e) {
      std::string errorMessage(e.what());
      fail("ERROR: " + errorMessage);
    }
  }
}

void
Nwd::FaceIdFetcher::onQueryFailure(uint32_t errorCode,
                                    const ndn::util::FaceUri& canonicalUri)
{
  std::stringstream ss;
  ss << "Cannot fetch data (code " << errorCode << ")";
  fail(ss.str());
}

void
Nwd::FaceIdFetcher::fail(const std::string& reason)
{
	m_onFail(reason);
	delete this;
}

void
Nwd::FaceIdFetcher::succeed(uint32_t faceId)
{
  
  m_onSucceed(faceId);
  
  delete this;
}

  void
  Nwd::FaceIdFetcher::startFaceCreate(const ndn::util::FaceUri& canonicalUri)
  {
	 ControlParameters parameters;
 	 parameters.setUri(canonicalUri.toString());

  	 m_controller.start<FaceCreateCommand>(parameters,
                                        [this] (const ControlParameters& result) {
                                          succeed(result.getFaceId());
                                        },
                                        bind(&FaceIdFetcher::onFaceCreateError, this, _1, _2,
                                             "Face creation failed"));
  }

 void
 Nwd::FaceIdFetcher::onFaceCreateError(uint32_t code,
                                       const std::string& error,
                                       const std::string& message)
 {
  	std::stringstream ss;
  	ss << message << " : " << error << " (code " << code << ")";
 	fail(ss.str());
 }



  }
}
