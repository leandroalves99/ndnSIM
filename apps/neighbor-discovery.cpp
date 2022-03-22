/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// custom-app.cpp
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <random>

#include "neighbor-discovery.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"

#include <ndn-cxx/lp/tags.hpp>
#include "ns3/mobility-model.h"


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include "ns3/ndnSIM-module.h"

NS_LOG_COMPONENT_DEFINE("NeighborDiscovery");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(NeighborDiscovery);

// register NS-3 type
TypeId
NeighborDiscovery::GetTypeId()
{
  static TypeId tid = TypeId("NeighborDiscovery").SetParent<ndn::App>().AddConstructor<NeighborDiscovery>();
  return tid;
}

NeighborDiscovery::NeighborDiscovery()
  : m_frequency(1.0/5)
  , m_firstTime(true)
  , m_seq(0)
  , m_maxHops(2)
  , m_maxDelay(0.09)  //50ms
{
  NS_LOG_FUNCTION_NOARGS();
}
NeighborDiscovery::~NeighborDiscovery()
{
}

void
NeighborDiscovery::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  //std::cout << "next: " << Simulator::Now().ToDouble(Time::S) << "s\n";
  
  //Simulator::Schedule(Seconds(1.0), &CustomApp::SendInterest, this);

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &NeighborDiscovery::SendInterest, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning()){

    m_sendEvent = Simulator::Schedule(Seconds(1.0 / m_frequency), &NeighborDiscovery::SendInterest, this);
  }
}
// Processing upon start of the application
void
NeighborDiscovery::StartApplication()
{
  // initialize ndn::App
  ndn::App::StartApplication();

  ndn::FibHelper::AddRoute(GetNode(), "/neighbor", m_face, 0);
  
  std::default_random_engine generator;
  std::uniform_int_distribution<int> distribution;
  generator.seed(chrono::steady_clock::now().time_since_epoch().count());
  int random = distribution(generator);
  srand (random);

  Vector positions = GetNode()->GetObject<ns3::MobilityModel>()->GetPosition();
  m_prevLocation = std::make_tuple(positions.x, positions.y, 0.0);
  m_prevTime = ndn::time::steady_clock().now();
  // Schedule send of first interest
  ScheduleNextPacket();;
}

// Processing when application is stopped
void
NeighborDiscovery::StopApplication()
{
  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);
  
  // cleanup ndn::App
  ndn::App::StopApplication();

}

void
NeighborDiscovery::SendInterest()
{
  if(!m_active) return;

  std::shared_ptr<ndn::Name> interestName = std::make_shared<ndn::Name>("/neighbor");
  interestName->appendNumber(GetNode()->GetId());
  // Create and configure ndn::Interest
  auto interest = std::make_shared<ndn::Interest>();
  interest->setName(*interestName);
  
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::milliseconds(5000)); //more than 3s should be enough 
  interest->setCanBePrefix(false);
  interest->setHopLimit(m_maxHops);

  //Search for getting NetDevice face ID
  Ptr<ndn::L3Protocol> ndn = GetNode()->GetObject<ndn::L3Protocol> ();
  Ptr<ns3::NetDevice> netDevice = GetNode()->GetDevice(0);  //get first NetDevice, only has one
  std::shared_ptr<ndn::Face> face = ndn->getFaceByNetDevice(netDevice); //get NetDevice face
  
  interest->setTag<ndn::lp::NextHopFaceIdTag>(std::make_shared<ndn::lp::NextHopFaceIdTag>(face->getId()));  //send directly to wifi face
  
  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  double x = ((double)std::rand()) / ((double)RAND_MAX) * m_maxDelay;
  Simulator::Schedule(Seconds(x), &NeighborDiscovery::SendInterestWithDelay, this, interest);

  ScheduleNextPacket();
}

// Callback that will be called when Interest arrives
void
NeighborDiscovery::OnInterest(std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest(interest);

  NS_LOG_DEBUG("Received Interest packet for " << interest->getName());
  // Note that Interests send out by the app will not be sent back to the app !
  
  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  
  ndn::StackHelper::getKeyChain().sign(*data);

  //Search for getting NetDevice face ID
  Ptr<ndn::L3Protocol> ndn = GetNode()->GetObject<ndn::L3Protocol> ();
  Ptr<ns3::NetDevice> netDevice = GetNode()->GetDevice(0);  //get first NetDevice, only has one
  std::shared_ptr<ndn::Face> face = ndn->getFaceByNetDevice(netDevice); //get NetDevice face
  
  data->setTag<ndn::lp::NextHopFaceIdTag>(std::make_shared<ndn::lp::NextHopFaceIdTag>(face->getId()));

  //Add NeighborTag to data 
  Vector positions = GetNode()->GetObject<ns3::MobilityModel>()->GetPosition();

  NS_LOG_DEBUG("Adding neighbor location[" << positions << "] to data");

  auto location = std::make_tuple(positions.x, positions.y, positions.z);
  
  auto neighborTag = std::make_pair(GetNode()->GetId(), location);    //pair<nodeID, nodeLocation>()
  data->setTag<ndn::lp::NeighborTag>(std::make_shared<ndn::lp::NeighborTag>(neighborTag));

  auto currentTime = ndn::time::steady_clock().now();
  if(currentTime >= m_prevTime) {
    NS_LOG_DEBUG("Saving current position...");
    m_prevTime += ndn::time::seconds(3);
    m_prevLocationCopy = m_prevLocation;
    m_prevLocation = location;
  }
  data->setTag<ndn::lp::GeoTag>(std::make_shared<ndn::lp::GeoTag>(m_prevLocationCopy));

  data->wireEncode();

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  //set random delay to send data
  double x = ((double)std::rand()) / ((double)RAND_MAX) * m_maxDelay;
  Simulator::Schedule(Seconds(x), &NeighborDiscovery::SendDataWithDelay, this, data);
}

void
NeighborDiscovery::SendInterestWithDelay(std::shared_ptr<const ndn::Interest> interest){
  NS_LOG_DEBUG("Sending Interest packet for " << *interest);

  m_appLink->onReceiveInterest(*interest);
}

void
NeighborDiscovery::SendDataWithDelay(std::shared_ptr<const ndn::Data> data){
  NS_LOG_DEBUG("Sending Data packet for " << data->getName());

  m_appLink->onReceiveData(*data);
}

// Callback that will be called when Data arrives
void
NeighborDiscovery::OnData(std::shared_ptr<const ndn::Data> data)
{
  if(!m_active) return;
  
  NS_LOG_DEBUG("Receiving Data packet for " << data->getName());
}

} // namespace ns3
