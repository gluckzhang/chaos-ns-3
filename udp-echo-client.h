/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright 2007 University of Washington
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

#ifndef UDP_ECHO_CLIENT_H
#define UDP_ECHO_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Socket;
class Packet;

class UdpEchoClient : public Application 
{
public:
  static TypeId GetTypeId (void);

  UdpEchoClient ();

  virtual ~UdpEchoClient ();

  void SetRemote (Address ip, uint16_t port);
  void SetRemote (Address addr);

  void SetDataSize (uint32_t dataSize);

  uint32_t GetDataSize (void) const;

  void SetFill (std::string fill);

 void SetFill (uint8_t fill, uint32_t dataSize);

 void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);

protected:
 virtual void DoDispose (void);

private:

 virtual void StartApplication (void);
 virtual void StopApplication (void);

 void ScheduleTransmit (Time dt);
 void Send (void);

 void HandleRead (Ptr<Socket> socket);

 uint32_t m_count; 
 Time m_interval; 
 uint32_t m_size; 
 
 uint32_t m_dataSize; 
 uint8_t *m_data; 

 uint32_t m_sent; 
 Ptr<Socket> m_socket; 
 Address m_peerAddress; 
 uint16_t m_peerPort; 
 EventId m_sendEvent; 
 
 TracedCallback<Ptr<UdpEchoClient>> Mym_txTrace;
 TracedCallback<Ptr<const Packet> > m_txTrace;

 typedef void (* SendNotifyCallback)(Ptr<UdpEchoClient> client);
};

} // namespace ns3

#endif /* UDP_ECHO_CLIENT_H */

