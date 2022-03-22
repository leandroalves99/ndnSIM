#include "neighborhood-strategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"
#include "common/global.hpp"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"

#include <ndn-cxx/lp/tags.hpp>
#include <cmath>
#include <string.h>

using namespace ns3;

namespace nfd {
namespace fw {

//NFD_REGISTER_STRATEGY(MulticastStrategy);

NFD_LOG_INIT(NeighborhoodStrategy);

NeighborhoodStrategy::NeighborhoodStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , entryLifetime(5_s)
          
{
  ParsedInstanceName parsed = parseInstanceName(name);
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
NeighborhoodStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/neighborhood/%FD%01");
  return strategyName;
}
void
NeighborhoodStrategy::onNitEntryExpires(neighbor_table::Entry *entry){
  entry->expiryTimer.cancel();
  this->getNit().eraseEntry(*entry);  //erase entry from NIT
}

void
NeighborhoodStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
    NFD_LOG_DEBUG(interest << " received");
    
    const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
    const fib::NextHopList& nexthops = fibEntry.getNextHops();
    for (const auto& nexthop : nexthops) {
      Face& outFace = nexthop.getFace();
      
      if ((outFace.getId() == ingress.face.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
          wouldViolateScope(ingress.face, interest, outFace)) {
        continue;
      }

      this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
      NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
      
    }
}

void
NeighborhoodStrategy::afterReceiveData(const shared_ptr<pit::Entry>& pitEntry, const FaceEndpoint& ingress, const Data& data)
{
  NFD_LOG_DEBUG("DATA RECEIVED with name " << data.getName());
  
  if(ingress.face.getLinkType() == ndn::nfd::LINK_TYPE_POINT_TO_POINT){
    //std::cout << "From Application" << std::endl;
    auto nextHopTag = data.getTag<lp::NextHopFaceIdTag>();
    if (nextHopTag != nullptr) {
      //chosen NextHop face exists?
      Face* nextHopFace = this->getFaceTable().get(*nextHopTag);
      if (nextHopFace != nullptr) {
        NFD_LOG_DEBUG("afterReceiveData data=" << data.getName()
                      << " nexthop-faceid=" << nextHopFace->getId());
        // go to outgoing Data pipeline
        // scope control is unnecessary, because privileged app explicitly wants to forward
      
        this->setExpiryTimer(pitEntry, 0_s);
        this->sendData(pitEntry, data, FaceEndpoint(*nextHopFace, 0));
      }
      return;
    }
    
  }

  std::shared_ptr<ndn::lp::NeighborTag> ntag = data.getTag<ndn::lp::NeighborTag>();
  std::shared_ptr<ndn::lp::GeoTag> ptag = data.getTag<ndn::lp::GeoTag>();
  std::tuple<double, double, double> position = std::get<1>(ntag->getNeighbor());
  std::tuple<double, double, double> prevPosition = ptag->getPos();
  NS_LOG_DEBUG("Node " << ntag->getNeighbor().first << " Current Position [ " << std::get<0>(position) <<", " << std::get<1>(position) << "]\tPrevious Position [" << std::get<0>(prevPosition) <<", " << std::get<1>(prevPosition) << "]");

  //insert neighbor's information in NIT
  auto entry = this->getNit().insert(ntag->getNeighbor(), prevPosition);
  entry->expiryTimer.cancel();
  entry->expiryTimer = getScheduler().schedule(entryLifetime, bind(&NeighborhoodStrategy::onNitEntryExpires, this, entry));
  
  //DEBUG purposes
  // NS_LOG_DEBUG("Neighbor Information table");
  // NS_LOG_DEBUG("Node ID" << std::setw(20) << "Current Position" << std::setw(20) << "Previous Position");
  // for (auto it = this->getNit().begin(); it != this->getNit().end(); it++) {
  //   NS_LOG_DEBUG(it->getId() << "\t [" << std::get<0>(it->getCurrentLocation()) <<", " << std::get<1>(it->getCurrentLocation()) << "]\t[" << std::get<0>(it->getPreviousLocation()) <<", " << std::get<1>(it->getPreviousLocation()) << "]");
  //   //std::cout <<it->getId() << "\t [" << std::get<0>(it->getCurrentLocation()) <<", " << std::get<1>(it->getCurrentLocation()) << "]\t[" << std::get<0>(it->getPreviousLocation()) <<", " << std::get<1>(it->getPreviousLocation()) << "]"<< std::endl;
  // }

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();

    //set Expiry time to PIT entry never be satisfied and send to application
    this->setExpiryTimer(pitEntry, 5_s);
    this->sendData(pitEntry, data, FaceEndpoint(outFace, 0));
  }
}

} // namespace fw
} // namespace nfd
