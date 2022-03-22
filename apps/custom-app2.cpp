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

#include "custom-app2.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"

#include <ndn-cxx/lp/tags.hpp>
#include "ns3/mobility-model.h"

NS_LOG_COMPONENT_DEFINE("CustomApp2");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(CustomApp2);

// register NS-3 type
TypeId
CustomApp2::GetTypeId()
{
  static TypeId tid = 
    TypeId("ns3::ndn::CustomApp2")
    .SetGroupName("Ndn")
    .SetParent<ndn::App>()
    .AddConstructor<CustomApp2>()

    .AddAttribute("NumberProducers", "Number of Producers",
                  IntegerValue(1),
                  MakeIntegerAccessor(&CustomApp2::m_nProducers), MakeIntegerChecker<uint32_t>())
    ;

  return tid;
}

CustomApp2::CustomApp2()
  : m_nProducers(0)
  , m_frequency(0.2)  //5 interests per second
  , m_txOffset(0.3) //200ms
  , m_firstTime(true)
  , m_seq(0)
  , m_retxTimer(MilliSeconds(100))
  , m_retxTimeout(false)
  , prevDistanceToProd({999.0, 999.0})
{
  NS_LOG_FUNCTION_NOARGS();
  
  //ScheduleRetxTimer(); 
}
CustomApp2::~CustomApp2()
{
}

void
CustomApp2::ScheduleRetxTimer()
{
  std::cout << "RetxTimer = " << m_retxTimer.GetMilliSeconds() << "ms" << std::endl;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &CustomApp2::CheckRetxTimeout, this);
}


void
CustomApp2::CheckRetxTimeout()
{
  //std::cout << "CheckRetxTimeout" << std::endl;
  Time now = Simulator::Now();

  Time rto(MilliSeconds(10000)); //retransmission timeout
  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::iterator entry = m_seqTimeouts.begin();
    if (entry->time + rto <= now) // timeout expired? 
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.erase(entry);
      OnTimeout(seqNo);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &CustomApp2::CheckRetxTimeout, this);
}

Time
CustomApp2::GetRetxTimer() const
{
  return m_retxTimer;
}

bool
CustomApp2::ShouldSendInterest(){
  bool send = false;
  if (m_nProducers == 1){
    double prevDistance[1];
    std::copy(std::begin(prevDistanceToProd), std::end(prevDistanceToProd), std::begin(prevDistance));
    Ptr<Node> producerNode1 = NodeList::GetNode(NodeList::GetNNodes() - 1);
    double currentDistanceTo1 = GetNode()->GetObject<ns3::MobilityModel>()->GetDistanceFrom(producerNode1->GetObject<ns3::MobilityModel>());
    //std::cout << GetNode()->GetId() << " | Current Distance: " << currentDistance << "| Previous Distance: " << prevDistanceToProd << std::endl;
    prevDistanceToProd[0] = currentDistanceTo1;

    if (currentDistanceTo1 < prevDistance[0] ){ // if close to  producer
      //std::cout << "Distance to 1/2 -> " << currentDistanceTo1 << " / " << currentDistanceTo2 << std::endl;
      //if (currentDistanceTo1 <= 200 || currentDistanceTo2 <= 200)
        send = true;
    } else {
      send = false;
    }
  }

  if (m_nProducers == 2){
    double prevDistance[2];
    std::copy(std::begin(prevDistanceToProd), std::end(prevDistanceToProd), std::begin(prevDistance));
    Ptr<Node> producerNode1 = NodeList::GetNode(NodeList::GetNNodes() - 2);
    Ptr<Node> producerNode2 = NodeList::GetNode(NodeList::GetNNodes() - 1);
    double currentDistanceTo1 = GetNode()->GetObject<ns3::MobilityModel>()->GetDistanceFrom(producerNode1->GetObject<ns3::MobilityModel>());
    double currentDistanceTo2 = GetNode()->GetObject<ns3::MobilityModel>()->GetDistanceFrom(producerNode2->GetObject<ns3::MobilityModel>());
    //std::cout << GetNode()->GetId() << " | Current Distance: " << currentDistance << "| Previous Distance: " << prevDistanceToProd << std::endl;
    prevDistanceToProd[0] = currentDistanceTo1;
    prevDistanceToProd[1] = currentDistanceTo2;

    if (currentDistanceTo1 < prevDistance[0] || currentDistanceTo2 < prevDistance[1] ){ // if close to 1st producer OR close to 2nd producer
      //std::cout << "Distance to 1/2 -> " << currentDistanceTo1 << " / " << currentDistanceTo2 << std::endl;
      //if (currentDistanceTo1 <= 200 || currentDistanceTo2 <= 200){
        send = true;
        //std::cout << "Node " << GetNode()->GetId() << " should send interest" << std::endl;
      //}
    } else {
      send = false;
    }
  }
  return send;
}
void
CustomApp2::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  //std::cout << "next: " << Simulator::Now().ToDouble(Time::S) << "s\n";
  
  //Simulator::Schedule(Seconds(1.0), &CustomApp::SendInterest, this);
  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0 + m_txOffset), &CustomApp2::SendInterest, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning()){
    //std::cout << "ELSE IF new send Event" << std::endl;
    m_sendEvent = Simulator::Schedule(Seconds(1.0 / m_frequency), &CustomApp2::SendInterest, this);
    //if(m_retxTimeout) m_retxTimeout = false;  //reset timeout retransmission
  }
}

// Processing upon start of the application
void
CustomApp2::StartApplication()
{
  // initialize ndn::App
  ndn::App::StartApplication();

  ScheduleNextPacket();
  
}

// Processing when application is stopped
void
CustomApp2::StopApplication()
{
  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);
  
  // cleanup ndn::App
  ndn::App::StopApplication();

}
void
CustomApp2::SendInterest()
{
  if(!m_active) return;

  //check if app should send interest
  if(!ShouldSendInterest()) {
    ScheduleNextPacket();
    return;
  }
  //std::cout << "Node " << GetNode()->GetId() << " is creating a new interest..." << std::endl;
  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
  NS_LOG_DEBUG("Pending Seqs: " << m_retxSeqs.size());
  if (!m_retxSeqs.empty()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    seq = m_seq++;
  }

  //Verify positions from producers
  Vector position = GetNode()->GetObject<MobilityModel> ()->GetPosition();

  // create name for interest
  std::shared_ptr<ndn::Name> interestName;
  // Add Producer location as GeoTag
  std::tuple<double, double, double> geoTag;
  
  if (m_nProducers == 1){
    Ptr<Node> producerNode1 = NodeList::GetNode(NodeList::GetNNodes() - 1);
    double distanceProd_1 = GetNode()->GetObject<ns3::MobilityModel>()->GetDistanceFrom(producerNode1->GetObject<ns3::MobilityModel>());

    interestName = std::make_shared<ndn::Name>("/parkinglot1");
    geoTag = std::make_tuple(200,100,0);
  }

  if (m_nProducers == 2){
    Ptr<Node> producerNode1 = NodeList::GetNode(NodeList::GetNNodes() - 2);
    Ptr<Node> producerNode2 = NodeList::GetNode(NodeList::GetNNodes() - 1);
    double distanceProd_1 = GetNode()->GetObject<ns3::MobilityModel>()->GetDistanceFrom(producerNode1->GetObject<ns3::MobilityModel>());
    double distanceProd_2 = GetNode()->GetObject<ns3::MobilityModel>()->GetDistanceFrom(producerNode2->GetObject<ns3::MobilityModel>());
    //interestName = std::make_shared<ndn::Name>("/parkinglot1");
    //geoTag = std::make_tuple(50,250,0);
    if(distanceProd_1 < distanceProd_2){
      interestName = std::make_shared<ndn::Name>("/parkinglot1");
      geoTag = std::make_tuple(50,250,0);
    }else{
      interestName = std::make_shared<ndn::Name>("/parkinglot2");
      geoTag = std::make_tuple(450,250,0);
    }
  }
  //add sequence number to interest name
  interestName->appendTimestamp();
  //interestName->appendSequenceNumber(seq);

  // std::cout << "Interest Name: " << *interestName << std::endl;
  // std::cout << "Interest Size: " << interestName->size() << std::endl;
  // std::cout << "Interest Sequence: " << interestName->get(-1).toSequenceNumber() << std::endl;
  
  // Create and configure ndn::Interest
  auto interest = std::make_shared<ndn::Interest>();
  interest->setName(*interestName);
  
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::milliseconds(1000));
  interest->setCanBePrefix(false);
  //interest->setHopLimit(5);

  //std::cout << "Node " << GetNode()->GetId() << " getting data from (" << std::get<0>(geoTag) << ", " << std::get<1>(geoTag) << ")" << std::endl;

  NS_LOG_DEBUG("Set producer with " << std::get<0>(geoTag) << ", " << std::get<1>(geoTag) << " coordinates");
  interest->setTag<ndn::lp::GeoTag>(std::make_shared<ndn::lp::GeoTag>(geoTag));

  //std::cout << "INTEREST sent with name " << interest->getName() << " and sequence number " << seq << std::endl;

  NS_LOG_DEBUG("Sending Interest packet for " << *interest);
  NS_LOG_DEBUG("> Interest for " << seq);
  //std::cout << GetNode()->GetId() << " | INTEREST sent with sequence " << seq << std::endl;
  //LOGGING PURPOSES
  //insert current interest in timeout sequence
  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqLastDelay.eraseBySequence(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqRetxCounts[seq]++;

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);
  //m_appLink->onReceiveInterest(*interest);

  double x = ((double)std::rand()) / ((double)RAND_MAX) * 0.05;
  Simulator::Schedule(Seconds(x), &CustomApp2::SendInterestWithDelay, this, interest);

  ScheduleNextPacket();
}

// Callback that will be called when Data arrives
void
CustomApp2::OnData(std::shared_ptr<const ndn::Data> data)
{
  if(!m_active) return;

  App::OnData(data);

  //std::cout << "Node " << GetNode()->GetId() << " received Data." << std::endl;

  NS_LOG_DEBUG("Receiving Data packet for " << data->getName());

  //uint32_t seq = data->getName().at(-1).toSequenceNumber();

  //std::cout << GetNode()->GetId() << " | DATA received for name " << data->getName() << " with sequence number " << seq << std::endl;

  int hopCount = 0;
  auto hopCountTag = data->getTag<ndn::lp::HopCountTag>();
  if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
    hopCount = *hopCountTag;
  }
  NS_LOG_DEBUG("Hop count: " << hopCount);

  // SeqTimeoutsContainer::iterator entry = m_seqLastDelay.findBySequence(seq);
  // if (entry != m_seqLastDelay.end()) {
  //   m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
  // }
  // entry = m_seqFullDelay.findBySequence(seq);
  // if (entry != m_seqFullDelay.end()) {
  //   m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
  // }

  // m_seqRetxCounts.erase(seq);
  // m_seqFullDelay.eraseBySequence(seq);
  // m_seqLastDelay.eraseBySequence(seq);

  // m_seqTimeouts.eraseBySequence(seq);
  // m_retxSeqs.erase(seq);
  
}

void
CustomApp2::OnTimeout(uint32_t sequenceNumber)
{
  NS_LOG_DEBUG("OnTimeout");
  NS_LOG_FUNCTION(sequenceNumber);
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  // m_rtt->IncreaseMultiplier(); // Double the next RTO
  // m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
  //                1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert(sequenceNumber);
  m_retxTimeout = true; // send new packet by timeout
  ScheduleNextPacket();
}

void
CustomApp2::SendInterestWithDelay(std::shared_ptr<const ndn::Interest> interest){
  NS_LOG_DEBUG("Sending Interest packet for " << *interest);

  m_appLink->onReceiveInterest(*interest);
}


} // namespace ns3
