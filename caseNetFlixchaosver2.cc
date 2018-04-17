#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/olsr-helper.h"
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <stdio.h>
#include <math.h> 
#include "ns3/vector.h"
#include "ns3/nqos-wifi-mac-helper.h"
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");
#define PI 3.14159265
//Global values
//Errorinjectiontypes if true then it will initiate that kind of errorinjection 
//static bool PreventDestruction = true;
static bool DoingChaosExperiment = false;
static bool DataRatefaultinject = false;
static bool P2PNodefaultinject = false;
static bool P2PDevicefaultinject = false;
static bool CSMADevicefaultinject = false;
static bool CSMANodefaultinject = false;
static bool STANodefaultinject = false;
static bool STADevicefaultinject = false;
static bool APNodefaultinject = false;
static bool APDevicefaultinject = false;
static bool RequestMapRoad = false;
static int StartNode;
static int EndNode;
static string ChaosPaths;
static vector<int> intChaosPaths;
static ApplicationContainer m_monkeys ;
static NodeContainer emptync;
static NetDeviceContainer emptynetdev;



struct MyNode{
	Ptr<Node> thisnode;
	vector <MyNode*> neighbor;
	vector<string> IPs;
};
	
static vector<MyNode*> allmynodes;	


class MyMonkey : public Application 
{
public:

  MyMonkey ():m_running (false){
		m_monkeys.Add(Ptr<MyMonkey>(this));
		}
  ~MyMonkey(){}
  void Setup(NodeContainer p2pnc,NetDeviceContainer p2pnetdev,NodeContainer csmanc,NetDeviceContainer csmanetdev,NodeContainer stanc,NetDeviceContainer stanetdev,NodeContainer apnc,NetDeviceContainer apnetdev){
	  m_p2pnodes = p2pnc;
	  m_p2pdevices = p2pnetdev;
	  m_csmanodes = csmanc;
	  m_csmadevices = csmanetdev;
	  m_stawifinodes = stanc;
	  m_stawifidevices = stanetdev;
	  m_apwifinodes = apnc;
	  m_apwifidevices = apnetdev;
	  m_nr = m_monkeys.GetN() + 1;
	}	
  
       uint32_t m_nr;
       NetDeviceContainer m_csmadevices;
       NetDeviceContainer m_p2pdevices;
       NetDeviceContainer m_stawifidevices;
       NetDeviceContainer m_apwifidevices;
       NodeContainer   m_p2pnodes;
       NodeContainer   m_csmanodes;
       NodeContainer   m_stawifinodes;
       NodeContainer   m_apwifinodes;
       EventId         m_errorInjectEvent;
       bool            m_running;
       
        
       void NodeDestroy(Ptr<Node> m_node,int nodenr){
          ostringstream ossinfo;
	  ossinfo << "Node nr "<< nodenr << " have been destroyed at time " << (Simulator::Now()).GetSeconds() << "\n";
          std::clog << ossinfo.str();
          m_node->Dispose();
        }
				
				void DeviceDestroy(Ptr<NetDevice> m_device,int devnr){
          ostringstream ossinfo;
	  ossinfo << "Device nr "<<  devnr <<" have been destroyed at time " << (Simulator::Now()).GetSeconds() << "\n";
          std::clog << ossinfo.str();
          m_device->Dispose();
        }


				void P2PChangeDataRate(Ptr<NetDevice> minp1){
	  			Ptr<PointToPointNetDevice> dev1 = minp1->GetObject<PointToPointNetDevice>();
          ostringstream ossinfo;
	  ossinfo << "At time " << (Simulator::Now()).GetSeconds() << "s " << " monkey "<< 		  m_nr <<" caused chaos to datarate of device " << dev1->GetIfIndex() << "\n";
          std::clog << ossinfo.str();
	  dev1->SetDataRate(DataRate("1Mbps"));
	}

        void ErrorInject(){
          if(DataRatefaultinject){
	    				P2PChangeDataRate(m_p2pdevices.Get(0));
          }
          if(P2PNodefaultinject){
            	NodeDestroy(m_p2pnodes.Get(0),0);
          }
          if(P2PDevicefaultinject){
            	DeviceDestroy(m_p2pdevices.Get(0),0);
          }
          if(CSMANodefaultinject){
            	NodeDestroy(m_csmanodes.Get(0),0);
          }
          if(CSMADevicefaultinject){
            	DeviceDestroy(m_csmadevices.Get(0),0);
          }
          if(STANodefaultinject){
            	NodeDestroy(m_stawifinodes.Get(0),0);
          }
          if(STADevicefaultinject){
            	DeviceDestroy(m_stawifidevices.Get(0),0);
          }
          if(APNodefaultinject){
            	NodeDestroy(m_apwifinodes.Get(0),0);
          }
          if(APDevicefaultinject){
            	DeviceDestroy(m_apwifidevices.Get(0),0);
          }
          
	}

	virtual void StartApplication (void)
	{
	  m_running = true;
	  ErrorInject();
	}

	virtual void StopApplication (void)
	{
	  m_running = false;

	  if (m_errorInjectEvent.IsRunning ())
	    {
	      Simulator::Cancel (m_errorInjectEvent);
	    }
	}


	void ScheduleTx (void)
	{
	  if (m_running)
	    {
	      Time tNext = Simulator::Now();
	      m_errorInjectEvent = Simulator::Schedule (tNext, &MyMonkey::ErrorInject,this);
	    }
	}

};



class MyObject : public Object
{
	public:
		/**
		 * Register this type.
		 * \return The TypeId.
		 */
		TracedValue<int32_t> m_myDataRate = 32768;
		static TypeId GetTypeId (void)
		{
		  static TypeId tid = TypeId ("MyObject")
		    .SetParent<Object> ()
		    .SetGroupName ("Tutorial")
		    .AddConstructor<MyObject> ()
		    .AddTraceSource ("DataRateChanges",
		                     "An integer value to trace.",
		                     MakeTraceSourceAccessor (&MyObject::m_myDataRate),
		                     "ns3::TracedValueCallback::Int32")
		  ;
		  return tid;
		}

		MyObject () {}
		void SetDataRate(DataRate data){
		  m_myDataRate = (int)data.GetBitRate();
		}

};


//void ChangeDataRate(Ptr<NetDevice> minp,string str){
//	Ptr<PointToPointNetDevice> dev = minp->GetObject<PointToPointNetDevice>();
//	dev->SetDataRate(DataRate(str));
//	Ptr<MyObject> ob = (dev->dataOb)->GetObject<MyObject>();
//	ob->SetDataRate(dev->GetBps());
//}


//----------------------------------------------------
//Your system for chaos injection
//Useful global methods

string GetNodeIP(Ptr<Ipv4> ipv4,uint32_t index){
	ostringstream oss;
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (index,0); 
  Ipv4Address ipAddr = iaddr.GetLocal (); 
  ipAddr.Print(oss);
  return oss.str();
}

bool FindIP(string IP,vector<string> vector){
    if ( find(vector.begin(), vector.end(), IP) != vector.end() ){
      return true;
    }
    return false;
}

NodeContainer BuildCsmaConnection(Ptr<Node> Sta,Ptr<Node> Ap){
  NodeContainer csmaNodes;
  csmaNodes.Add(Sta);
  csmaNodes.Add(Ap);

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (csmaNodes);
  
  return csmaNodes;
}

NetDeviceContainer BuildP2PConnection(Ptr<Node> Sta,Ptr<Node> Ap,Ipv4AddressHelper& address){
  NodeContainer p2pNodes;
	p2pNodes.Add(Sta);
	p2pNodes.Add(Ap);

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer p2pDevices;
	p2pDevices = pointToPoint.Install (p2pNodes);
  
	Ipv4InterfaceContainer p2pInterfaces;
	p2pInterfaces = address.Assign (p2pDevices);
  
  return p2pDevices;
}
void Message(string msg){
  clog << msg << endl;
}

void BuildEchoServer(Ptr<Node> servernode,int port){
  UdpEchoServerHelper echoServer (port);
	ApplicationContainer serverApps = echoServer.Install (servernode);
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (10.0));
}

void BuildEchoClient(Ptr<Node> clientnode,Ipv4Address address,int port,double starttime){
  UdpEchoClientHelper echoClient (address, port);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps = echoClient.Install (clientnode);
	clientApps.Start (Seconds (starttime));
	clientApps.Stop (Seconds (10.0));
	
	uint32_t servernodeid;
	ostringstream oss; 
  address.Print(oss);
  
	for(auto elem : allmynodes){
	  if(FindIP(oss.str(),elem->IPs)){
	    servernodeid = elem->thisnode->GetId();
	  }
	}
	
	oss.str("");
	oss.clear();
	oss << "[PlanedSendEvent] At time " << starttime << "s EchoClientNode " << clientnode->GetId() << " Send packages to EchoServerNode " << servernodeid ;
	Simulator::Schedule(Time(Seconds(starttime)),&Message,oss.str());
}


class System{

public:
        ~System(){};
        System(){}
	static void ObjectDestroyCallBack(Ptr<Object> obptr){
	  obptr->DisableObjectDestruction();
    ostringstream ossinfo;
		ossinfo << "Object destroyed!!!!!" << "\n";
    NS_LOG_INFO(ossinfo.str());
	}

	void SetMyObject(Ptr<NetDevice> minp,Ptr<MyObject> ob){
		Ptr<PointToPointNetDevice> minp2 = minp->GetObject<PointToPointNetDevice>();
		ob->SetDataRate(minp2->GetBps());
		minp2->SetDataOb(ob);
	}

	static void IntTrace (int32_t oldValue, int32_t newValue)
	{       ostringstream ossinfo;
		ossinfo << "A monkey had caused chaos!DataRateChanges! Traced " << oldValue << " to " << newValue << std::endl;
                NS_LOG_INFO(ossinfo.str());
	}

	static void FixDataRate ( Ptr<PointToPointNetDevice> pointer){
		if((pointer->GetBps()).GetBitRate()!=DataRate("5Mbps").GetBitRate()){
		  Ptr<Node> node = pointer->GetNode();
                  ostringstream ossinfo1;
		  ossinfo1 << "System detected a chaos monkey at Node "<< node->GetId() <<" Device " << pointer->GetIfIndex() <<" !DataRate changed to "<< (pointer->GetBps()).GetBitRate()  <<"\n" ;
                  NS_LOG_INFO(ossinfo1.str());
                  ostringstream ossinfo2;
		  pointer->SetDataRate(DataRate("5Mbps"));
		  ossinfo2 << "DataRate fixed back to default!!!!!"<< (pointer->GetBps()).GetBitRate()  <<"\n" ;
                  NS_LOG_INFO(ossinfo2.str());	
	  }
	}
	
  NetDeviceContainer installP2PDevice(NodeContainer nc,PointToPointHelper p2phelper,vector<MyNode*>& allmynodes){
    bool registered1 = false;
    bool registered2 = false;
    MyNode* mnode1 = new MyNode;
    MyNode* mnode2 = new MyNode;
    for(auto elem: allmynodes){
      if((nc.Get(0))->GetId() == (elem->thisnode)->GetId()){
        registered1 = true;
        mnode1 = elem;
      }
      if((nc.Get(1))->GetId() == (elem->thisnode)->GetId()){
        registered2 = true;
        mnode2 = elem;
      }
    }
    if(!registered1){
    	mnode1->thisnode = nc.Get(0);
    	allmynodes.push_back(mnode1);
    }
    if(!registered2){
      mnode2->thisnode = nc.Get(1);
      allmynodes.push_back(mnode2);
    }
  	mnode1->neighbor.push_back(mnode2);
    mnode2->neighbor.push_back(mnode1);
		
    return p2phelper.Install(nc);
  }
  
  void AddChildren(vector<MyNode*>& children,vector<MyNode*> addition,vector<int> visited){
    for( auto elem: addition){
      if(!CheckVisited(elem->thisnode->GetId(),visited)){
        children.push_back(elem);
      }
    }
  }
  
  bool CheckVisited(int nodeid,vector<int> visited){
    if ( find(visited.begin(), visited.end(), nodeid) != visited.end() ){
      return true;
    }
    return false;
  }
  
  void MapRoad(int start,int end,vector<MyNode*>& allmynodes,vector<int> visited,string currentroad,vector<string>& roads){
	  currentroad += to_string(start) + ",";
	  MyNode *currentnode = allmynodes[start];
	  visited.push_back(start);
	  for( auto elem : currentnode->neighbor){
	    int id = elem->thisnode->GetId();
	    if( id != end){
				if(!CheckVisited(id,visited)){
				  MapRoad(id,end,allmynodes,visited,currentroad,roads);
				}
		  }
		  else{
		    roads.push_back(currentroad + to_string(id));
		  }
	  }
  }

  
  Ipv4InterfaceContainer AssignP2PAddress(Ipv4AddressHelper address,NetDeviceContainer devnc){
    Ipv4InterfaceContainer con = address.Assign(devnc);
    Ptr<Node> node1 = devnc.Get(0)->GetNode();
    Ptr<Node> node2 = devnc.Get(1)->GetNode();
    MyNode* mynode1 = allmynodes[node1->GetId()];
    MyNode* mynode2 = allmynodes[node2->GetId()];
    mynode1->IPs.push_back(GetNodeIP(con.Get(0).first,con.Get(0).second));
    mynode2->IPs.push_back(GetNodeIP(con.Get(1).first,con.Get(1).second));
    return con;
  }
  
  void MapRoadFromIP(string startIP,string endIP,vector<MyNode*>& allmynodes,vector<int> visited,string currentroad,vector<string>& roads){
    int start;
    int end;
    MyNode* currentnode;
    for(auto elem: allmynodes){
      currentnode = elem;
      if( FindIP(startIP,elem->IPs)){
        start = currentnode->thisnode->GetId();
      }
      if( FindIP(endIP,elem->IPs)){
        end = currentnode->thisnode->GetId();
      }
    }
    cout << "Start : " << start << " End : " << end << endl;
    MapRoad(start,end,allmynodes,visited,currentroad,roads);
  }
  
  void SetDownNode(Ptr<Node> node){
    Ptr<Ipv4> ip = node->GetObject<Ipv4>();
	  for(uint32_t j=0; j < ip->GetNInterfaces(); ++j){
	    ip->SetDown(j);
	  }
  }
  
	void RunSystem ()
	{     
				vector<string> roads;
				vector<int> visited;
				if (true)
					{
					  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
					  LogComponentEnable ("UdpEchoClientApplication", LOG_PREFIX_ALL);
					  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
					  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
					}
				

				NodeContainer p2pNodes1,p2pNodes2,p2pNodes3,p2pNodes4,p2pNodes5,p2pNodes6;
				p2pNodes1.Create(2);
				p2pNodes2.Add(p2pNodes1.Get(0));
				p2pNodes2.Create(1);
				p2pNodes3.Add(p2pNodes1.Get(0));
				p2pNodes3.Create(1);
				p2pNodes4.Add(p2pNodes1.Get(1));
				p2pNodes4.Create(1);
				p2pNodes5.Add(p2pNodes2.Get(1));
				p2pNodes5.Add(p2pNodes4.Get(1));
				p2pNodes6.Add(p2pNodes3.Get(1));
				p2pNodes6.Add(p2pNodes4.Get(1));
				
				InternetStackHelper internet;
				internet.Install (p2pNodes1);
				internet.Install (p2pNodes6);
				internet.Install (p2pNodes5.Get(0));
				
				PointToPointHelper pointToPoint;
				pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
				pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

				NetDeviceContainer p2pDevices1,p2pDevices2,p2pDevices3,p2pDevices4,p2pDevices5,p2pDevices6;
				p2pDevices1 = installP2PDevice(p2pNodes1,pointToPoint,allmynodes);
				p2pDevices2 = installP2PDevice(p2pNodes2,pointToPoint,allmynodes);
				p2pDevices3 = installP2PDevice(p2pNodes3,pointToPoint,allmynodes);
				p2pDevices4 = installP2PDevice(p2pNodes4,pointToPoint,allmynodes);
				p2pDevices5 = installP2PDevice(p2pNodes5,pointToPoint,allmynodes);
				p2pDevices6 = installP2PDevice(p2pNodes6,pointToPoint,allmynodes);

				Ipv4AddressHelper address;
        
				Ipv4InterfaceContainer p2pInterfaces1,p2pInterfaces2,p2pInterfaces3,p2pInterfaces4,p2pInterfaces5,p2pInterfaces6;	 
				address.SetBase ("10.1.1.0", "255.255.255.0");
				p2pInterfaces1 = AssignP2PAddress(address,p2pDevices1);
				address.SetBase ("10.1.2.0", "255.255.255.0");
				p2pInterfaces2 = AssignP2PAddress(address,p2pDevices2);
				address.SetBase ("10.1.3.0", "255.255.255.0");
				p2pInterfaces3 = AssignP2PAddress(address,p2pDevices3);
				address.SetBase ("10.1.4.0", "255.255.255.0");
				p2pInterfaces4 = AssignP2PAddress(address,p2pDevices4);
				address.SetBase ("10.1.5.0", "255.255.255.0");
				p2pInterfaces5 = AssignP2PAddress(address,p2pDevices5);
				address.SetBase ("10.1.6.0", "255.255.255.0");
				p2pInterfaces6 = AssignP2PAddress(address,p2pDevices6);
					
        MobilityHelper mobility;
				mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
												               "MinX", DoubleValue (0.0),
												               "MinY", DoubleValue (0.0),
												               "DeltaX", DoubleValue (5.0),
												               "DeltaY", DoubleValue (5.0),
												               "GridWidth", UintegerValue (3),
												               "LayoutType", StringValue ("RowFirst"));
						                     
			  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
				mobility.Install (p2pNodes1);
				mobility.Install(p2pNodes6);
				mobility.Install(p2pNodes5.Get(0));
				
				//Placing Nodes
			  static NodeContainer allnodes = NodeContainer::GetGlobal();
				AnimationInterface anim("NetFlixAnimver2.xml");		
				
				double r = 20;
				double angle = PI/4;
				anim.SetConstantPosition(allnodes.Get(0),0,0);
				anim.SetConstantPosition(allnodes.Get(1),r*cos(angle),r*sin(angle));
				anim.SetConstantPosition(allnodes.Get(2),r*cos(angle),0);
				anim.SetConstantPosition(allnodes.Get(3),r*cos(angle),-r*sin(angle));
				anim.SetConstantPosition(allnodes.Get(4),2*r*cos(angle),0);
				
				for(uint32_t i=0; i < allnodes.GetN() ;++i){
				  anim.UpdateNodeColor (allnodes.Get(i), 0, 255, 0); 
				}
				
				BuildEchoServer(p2pNodes4.Get(1),9);
				BuildEchoClient(p2pNodes1.Get(0),p2pInterfaces4.GetAddress(1), 9,2.0);
				Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
			  
			  if(!intChaosPaths.empty()){
					for( int elem : intChaosPaths){
					  SetDownNode(allnodes.Get(elem));
					  anim.UpdateNodeColor(allnodes.Get(elem), 255, 0, 0); 
				  }
			  }
			  
				Ipv4GlobalRoutingHelper::RecomputeRoutingTables ();
				
				if(RequestMapRoad){
					MapRoad(StartNode,EndNode,allmynodes,visited,"",roads);
					clog << "[Roads from Node " << StartNode << " to Node "<< EndNode << "]:";
					for(auto elem: roads){
						clog << elem << "|";
					}
					clog << "\n";
				}
				Simulator::Run ();
				Simulator::Destroy ();
		
	}
};

vector<string> split(const string& str, const string& delim)
{
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}

vector<int> ConvertStringToInt(vector<string> stringvec){
  vector<int> intvec;
  for(string elem : stringvec){
    intvec.push_back(atoi(elem.c_str()));
  }
  return intvec;
}
int main (int argc, char *argv[])
{ Time::SetResolution (Time::NS);
  LogComponentEnable ("SecondScriptExample", LOG_LEVEL_ALL);
  LogComponentEnable ("SecondScriptExample", LOG_PREFIX_ALL);
  
  CommandLine cmd;
  
  cmd.AddValue("DataRatefaultinject","Inject fault to datarate to a device",DataRatefaultinject);
  cmd.AddValue("P2PNodefaultinject","Destroy a p2p node",P2PNodefaultinject);
  cmd.AddValue("CSMANodefaultinject","Destroy a csma node",CSMANodefaultinject);
  cmd.AddValue("STANodefaultinject","Destroy a sta node",STANodefaultinject);
  cmd.AddValue("APNodefaultinject","Destroy a ap node",APNodefaultinject);
  cmd.AddValue("P2PDevicefaultinject","Destroy a p2p device",P2PDevicefaultinject);
  cmd.AddValue("CSMADevicefaultinject","Destroy a csma device",CSMADevicefaultinject);
  cmd.AddValue("STADevicefaultinject","Destroy a sta device",STADevicefaultinject);
  cmd.AddValue("APDevicefaultinject","Destroy a ap device",APDevicefaultinject);
  cmd.AddValue("DoingChaosExperiment","Enable chaos experiment",DoingChaosExperiment);
  cmd.AddValue("ChaosPaths","add a chaospath",ChaosPaths);
  cmd.AddValue("RequestMapRoad","Map a road from startnode to endnode",RequestMapRoad);
  cmd.AddValue("StartNode","StartNode",StartNode);
  cmd.AddValue("EndNode","EndNode",EndNode);
  
  cmd.Parse (argc, argv);
  vector<string> Mypaths = split(ChaosPaths,",");
  intChaosPaths = ConvertStringToInt(Mypaths);
  
  
  System mysyst;
  mysyst.RunSystem();
  if(DoingChaosExperiment){
     NS_LOG_INFO( "EXPERIMENT SUCCESS!" <<"\n");
  }
          

  return 0;
}







