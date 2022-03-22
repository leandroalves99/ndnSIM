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

// custom-app.hpp

#ifndef CUSTOM_APP_2_H_
#define CUSTOM_APP_2_H_

#include "ndn-app.hpp"

namespace ns3 {

/**
 * @brief A simple custom application
 *
 * This applications demonstrates how to send Interests and respond with Datas to incoming interests
 *
 * When application starts it "sets interest filter" (install FIB entry) for /prefix/sub, as well as
 * sends Interest for this prefix
 *
 * When an Interest is received, it is replied with a Data with 1024-byte fake payload
 */
class CustomApp2 : public ndn::App {
public:
  // register NS-3 type "CustomApp"
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  CustomApp2();
  virtual ~CustomApp2();

  // (overridden from ndn::App) Processing upon start of the application
  virtual void
  StartApplication();

  // (overridden from ndn::App) Processing when application is stopped
  virtual void
  StopApplication();

  // (overridden from ndn::App) Callback that will be called when Data arrives
  virtual void
  OnData(std::shared_ptr<const ndn::Data> contentObject);

  /**
 * @brief Timeout event
 * @param sequenceNumber time outed sequence number
 */
  virtual void
  OnTimeout(uint32_t sequenceNumber);

public:
  typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
  typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

private:
  void
  SendInterest();

protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();

  /**
   * \brief Checks if the packet need to be retransmitted becuase of retransmission timer expiration
   */
  void
  CheckRetxTimeout();

  /**
   * \brief Prepare scheduling of retransmission packets
   */
  void
  ScheduleRetxTimer();

  /**
   * \brief Decide if transmit interest or not
   */
  bool
  ShouldSendInterest();

  /**
   * \brief Returns the frequency of checking the retransmission timeouts
   * \return Timeout defining how frequent retransmission timeouts should be checked
   */
  Time
  GetRetxTimer() const;

  virtual void
  SendInterestWithDelay(std::shared_ptr<const ndn::Interest> interest);

protected:
  uint32_t m_nProducers;
  double m_frequency; // Frequency of interest packets (in hertz)
  double m_txOffset;  // transmission offset
  bool m_firstTime;   //Is first time to send interest
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  uint32_t m_seq;      ///< @brief currently requested sequence number
  Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
  EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed
  bool m_retxTimeout; // retransmit by timeout
  EventId m_retxTimeoutEvent; ///< @brief Event to perform retransmission by timeout
  double prevDistanceToProd[2];
  /**
   * \struct This struct contains sequence numbers of packets to be retransmitted
   */
  struct RetxSeqsContainer : public std::set<uint32_t> {
  };

  RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;

    bool operator==( const SeqTimeout& rhs)
	  {
		  return seq == rhs.seq;
	  }

    bool operator>( const SeqTimeout& rhs)
	  {
		  return seq > rhs.seq;
	  }

    bool operator<(const SeqTimeout& rhs)
	  {
		  return seq < rhs.seq;
	  }

  };

  struct CmpSeqTimeout
  {
    bool operator()(const SeqTimeout& lhs, const SeqTimeout& rhs) const
    {
        return (lhs.time < rhs.time);
    }
  };
  /**
   * \struct This struct contains a set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer : public std::set<SeqTimeout, CmpSeqTimeout> {
    void eraseBySequence(uint32_t seq){
      auto it = std::find_if(this->begin(), this->end(), [&](const SeqTimeout& p ){ return p.seq == seq; });
      if (it != this->end()) this->erase(it);
    }

    SeqTimeoutsContainer::iterator findBySequence(uint32_t seq){
      auto it = std::find_if(this->begin(), this->end(), [&](const SeqTimeout& p ){ return p.seq == seq; });
      if (it != this->end()) return it;
      return this->end();
    }
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief ordered set of sequence numbers to be retransmitted

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

};

} // namespace ns3

#endif // CUSTOM_APP_H_
