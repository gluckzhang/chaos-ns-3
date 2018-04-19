/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2007, 2008 University of Washington
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/net-device-queue-interface.h"
#include "point-to-point-net-device.h"
#include "point-to-point-channel.h"
#include "ppp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointNetDevice");

NS_OBJECT_ENSURE_REGISTERED (PointToPointNetDevice);

TypeId 
PointToPointNetDevice::GetTypeId (void)
{
 static TypeId tid = TypeId ("ns3::PointToPointNetDevice")
   .SetParent<NetDevice> ()
   .SetGroupName ("PointToPoint")
   .AddConstructor<PointToPointNetDevice> ()
   .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                  UintegerValue (DEFAULT_MTU),
                  MakeUintegerAccessor (&PointToPointNetDevice::SetMtu,
                                        &PointToPointNetDevice::GetMtu),
                  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("Address", 
                  "The MAC address of this device.",
                  Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                  MakeMac48AddressAccessor (&PointToPointNetDevice::m_address),
                  MakeMac48AddressChecker ())
   .AddAttribute ("DataRate", 
                  "The default data rate for point to point links",
                  DataRateValue (DataRate ("32768b/s")),
                  MakeDataRateAccessor (&PointToPointNetDevice::m_bps),
                  MakeDataRateChecker ())
   .AddAttribute ("ReceiveErrorModel", 
                  "The receiver error model used to simulate packet loss",
                  PointerValue (),
                  MakePointerAccessor (&PointToPointNetDevice::m_receiveErrorModel),
                  MakePointerChecker<ErrorModel> ())
   .AddAttribute ("InterframeGap", 
                  "The time to wait between packet (frame) transmissions",
                  TimeValue (Seconds (0.0)),
                  MakeTimeAccessor (&PointToPointNetDevice::m_tInterframeGap),
                  MakeTimeChecker ())

   //
   // Transmit queueing discipline for the device which includes its own set
   // of trace hooks.
   //
   .AddAttribute ("TxQueue", 
                  "A queue to use as the transmit queue in the device.",
                  PointerValue (),
                  MakePointerAccessor (&PointToPointNetDevice::m_queue),
                  MakePointerChecker<Queue<Packet> > ())

   //
   // Trace sources at the "top" of the net device, where packets transition
   // to/from higher layers.
   //
   .AddTraceSource ("MacTx", 
                    "Trace source indicating a packet has arrived "
                    "for transmission by this device",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_macTxTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("MacTxDrop", 
                    "Trace source indicating a packet has been dropped "
                    "by the device before transmission",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_macTxDropTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("MacPromiscRx", 
                    "A packet has been received by this device, "
                    "has been passed up from the physical layer "
                    "and is being forwarded up the local protocol stack.  "
                    "This is a promiscuous trace,",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_macPromiscRxTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("MacRx", 
                    "A packet has been received by this device, "
                    "has been passed up from the physical layer "
                    "and is being forwarded up the local protocol stack.  "
                    "This is a non-promiscuous trace,",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_macRxTrace),
                    "ns3::Packet::TracedCallback")
#if 0
   // Not currently implemented for this device
   .AddTraceSource ("MacRxDrop", 
                    "Trace source indicating a packet was dropped "
                    "before being forwarded up the stack",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_macRxDropTrace),
                    "ns3::Packet::TracedCallback")
#endif
   //
   // Trace souces at the "bottom" of the net device, where packets transition
   // to/from the channel.
   //
   .AddTraceSource ("PhyTxBegin", 
                    "Trace source indicating a packet has begun "
                    "transmitting over the channel",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyTxBeginTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("PhyTxEnd", 
                    "Trace source indicating a packet has been "
                    "completely transmitted over the channel",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyTxEndTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("PhyTxDrop", 
                    "Trace source indicating a packet has been "
                    "dropped by the device during transmission",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyTxDropTrace),
                    "ns3::Packet::TracedCallback")
#if 0
   // Not currently implemented for this device
   .AddTraceSource ("PhyRxBegin", 
                    "Trace source indicating a packet has begun "
                    "being received by the device",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxBeginTrace),
                    "ns3::Packet::TracedCallback")
#endif
   .AddTraceSource ("PhyRxEnd", 
                    "Trace source indicating a packet has been "
                    "completely received by the device",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxEndTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("PhyRxDrop", 
                    "Trace source indicating a packet has been "
                    "dropped by the device during reception",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_phyRxDropTrace),
                    "ns3::Packet::TracedCallback")

   //
   // Trace sources designed to simulate a packet sniffer facility (tcpdump).
   // Note that there is really no difference between promiscuous and 
   // non-promiscuous traces in a point-to-point link.
   //
   .AddTraceSource ("Sniffer", 
                   "Trace source simulating a non-promiscuous packet sniffer "
                    "attached to the device",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_snifferTrace),
                    "ns3::Packet::TracedCallback")
   .AddTraceSource ("PromiscSniffer", 
                    "Trace source simulating a promiscuous packet sniffer "
                    "attached to the device",
                    MakeTraceSourceAccessor (&PointToPointNetDevice::m_promiscSnifferTrace),
                    "ns3::Packet::TracedCallback")
 ;
 return tid;
}


PointToPointNetDevice::PointToPointNetDevice () 
 :
   m_txMachineState (READY),
   m_channel (0),
   m_linkUp (false),
   m_currentPkt (0)
{
 NS_LOG_FUNCTION (this);
}

PointToPointNetDevice::~PointToPointNetDevice ()
{
 NS_LOG_FUNCTION (this);
}

void
PointToPointNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
{
 NS_LOG_FUNCTION (this << p << protocolNumber);
 PppHeader ppp;
 ppp.SetProtocol (EtherToPpp (protocolNumber));
 p->AddHeader (ppp);
}

bool
PointToPointNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
 NS_LOG_FUNCTION (this << p << param);
 PppHeader ppp;
 p->RemoveHeader (ppp);
 param = PppToEther (ppp.GetProtocol ());
 return true;
}

void
PointToPointNetDevice::DoInitialize (void)
{
 if (m_queueInterface)
   {
     NS_ASSERT_MSG (m_queue != 0, "A Queue object has not been attached to the device");

     // connect the traced callbacks of m_queue to the static methods provided by
     // the NetDeviceQueue class to support flow control and dynamic queue limits.
     // This could not be done in NotifyNewAggregate because at that time we are
     // not guaranteed that a queue has been attached to the netdevice
     m_queueInterface->ConnectQueueTraces (m_queue, 0);
   }

 NetDevice::DoInitialize ();
}

void
PointToPointNetDevice::NotifyNewAggregate (void)
{
 NS_LOG_FUNCTION (this);
 if (m_queueInterface == 0)
   {
     Ptr<NetDeviceQueueInterface> ndqi = this->GetObject<NetDeviceQueueInterface> ();
     //verify that it's a valid netdevice queue interface and that
     //the netdevice queue interface was not set before
     if (ndqi != 0)
       {
         m_queueInterface = ndqi;
       }
   }
 NetDevice::NotifyNewAggregate ();
}

void
PointToPointNetDevice::DoDispose ()
{
 NS_LOG_FUNCTION (this);
 m_node = 0;
 m_channel = 0;
 m_receiveErrorModel = 0;
 m_currentPkt = 0;
 m_queue = 0;
