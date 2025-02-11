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

#include "custom-app.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"

#include <ndn-cxx/lp/tags.hpp>
#include "ns3/mobility-model.h"

NS_LOG_COMPONENT_DEFINE("CustomApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(CustomApp);

// register NS-3 type
TypeId
CustomApp::GetTypeId()
{
  static TypeId tid = TypeId("CustomApp").SetParent<ndn::App>().AddConstructor<CustomApp>();
  return tid;
}

CustomApp::CustomApp()
  : m_frequency(1.0)
  , m_firstTime(true)
  , m_seq(0)
{
  NS_LOG_FUNCTION_NOARGS();
}
CustomApp::~CustomApp()
{
}

void
CustomApp::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  //std::cout << "next: " << Simulator::Now().ToDouble(Time::S) << "s\n";
  
  //Simulator::Schedule(Seconds(1.0), &CustomApp::SendInterest, this);
  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &CustomApp::SendInterest, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning()){
    m_sendEvent = Simulator::Schedule(Seconds(1.0 / m_frequency), &CustomApp::SendInterest, this);
  }
}
// Processing upon start of the application
void
CustomApp::StartApplication()
{
  // initialize ndn::App
  ndn::App::StartApplication();

  // Schedule send of first interest
  ScheduleNextPacket();;
}

// Processing when application is stopped
void
CustomApp::StopApplication()
{
  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);
  
  // cleanup ndn::App
  ndn::App::StopApplication();

}

void
CustomApp::SendInterest()
{
  /////////////////////////////////////
  // Sending Interest packet out     //
  /////////////////////////////////////

  //create interest with producer location
  std::string producerLocation = "/200/100"; // /x_coord/y_coord -> /lat/lng
  std::shared_ptr<ndn::Name> interestName = std::make_shared<ndn::Name>(producerLocation);

  //append application name
  std::string applicationName = "named";
  interestName->append(applicationName);

  //add sequence number to interest name
  uint32_t seq = m_seq;
  interestName->appendSequenceNumber(seq);
  m_seq++;

  std::cout << "Interest Name: " << *interestName << std::endl;
  std::cout << "Interest Size: " << interestName->size() << std::endl;
  std::cout << "Interest GetSubName: " << interestName->getSubName(2,1) << std::endl;
  std::cout << "Interest GetPrefix(2): " << interestName->getPrefix(2) << std::endl;

  // Create and configure ndn::Interest
  auto interest = std::make_shared<ndn::Interest>();
  interest->setName(*interestName);
  
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::milliseconds(2000));
  interest->setCanBePrefix(false);

  // Add Consumer location as GeoTag
  Vector positions = GetNode()->GetObject<ns3::MobilityModel>()->GetPosition();

  NS_LOG_DEBUG("Adding GeoTag with positions[" << positions << "]");

  auto geoTag = std::make_tuple(positions.x, positions.y, positions.z);
  interest->setTag<ndn::lp::GeoTag>(std::make_shared<ndn::lp::GeoTag>(geoTag));;
  

  //TO DELETE
  auto neighborTag = std::make_pair(GetNode()->GetId(), geoTag);
  interest->setTag<ndn::lp::NeighborTag>(std::make_shared<ndn::lp::NeighborTag>(neighborTag));


  NS_LOG_DEBUG("Sending Interest packet for " << *interest);
  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  m_appLink->onReceiveInterest(*interest);

  ScheduleNextPacket();
}

// Callback that will be called when Interest arrives
void
CustomApp::OnInterest(std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest(interest);

  NS_LOG_DEBUG("Received Interest packet for " << interest->getName());
  std::cout << "INTEREST received for name " << interest->getName() << std::endl;
  // Note that Interests send out by the app will not be sent back to the app !

  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending Data packet for " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

// Callback that will be called when Data arrives
void
CustomApp::OnData(std::shared_ptr<const ndn::Data> data)
{
  NS_LOG_DEBUG("Receiving Data packet for " << data->getName());

  std::cout << "DATA received for name " << data->getName() << std::endl;
}

} // namespace ns3
