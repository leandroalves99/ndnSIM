#include "best-neighbor-strategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"

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

NFD_LOG_INIT(BestNeighborStrategy);

BestNeighborStrategy::BestNeighborStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , m_nRelays(1)
          
{
  ParsedInstanceName parsed = parseInstanceName(name);
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
BestNeighborStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/bestneighbor/%FD%01");
  return strategyName;
}

std::tuple<double, double, double>
BestNeighborStrategy::getProducerLocation(const Interest& interest)
{
    std::shared_ptr<ndn::lp::GeoTag> producerTag = interest.getTag<ndn::lp::GeoTag>();
    return std::make_tuple(std::get<0>(producerTag->getPos()), std::get<1>(producerTag->getPos()), 0);
}

double
calculateDistanceToProducer(std::tuple<double, double, double> v, std::tuple<double, double, double> prod)
{  
    double v_x, v_y, prod_x, prod_y;
    std::tie(v_x, v_y, std::ignore) = v;
    std::tie(prod_x, prod_y, std::ignore) = prod;

    double distance = sqrt(pow(prod_x - v_x, 2) + pow(prod_y - v_y, 2));
    return distance;
}

std::vector<uint32_t>
BestNeighborStrategy::performSelectionOfBestNeighbor(const Interest& interest){
  //current node position
  auto node = ns3::NodeList::GetNode(Simulator::GetContext());
  Vector currentPosition = node->GetObject<MobilityModel> ()->GetPosition();
  //producer position
  std::tuple<double, double, double> producerLoc = getProducerLocation(interest);
  
  std::vector<uint32_t> relays;
  std::vector<std::pair<uint32_t, double>> goodRelays;
  std::pair<uint32_t, double> currentNode;
  //initial values of selectedNeighbor are the current node (Id, Position)
  double prevDistanceToProducer = 0; 
  double distanceToProducer = calculateDistanceToProducer(std::make_tuple(currentPosition.x, currentPosition.y, 0), producerLoc);
  currentNode = std::make_pair(node->GetId(), distanceToProducer);

  //search for the best neighbor, neighbor closest to the producer location
  for (auto it = this->getNit().begin(); it != this->getNit().end(); it++) {
    NFD_LOG_DEBUG(it->getId() << "\t [" << std::get<0>(it->getCurrentLocation()) <<", " << std::get<1>(it->getCurrentLocation()) << "]\t[" << std::get<0>(it->getPreviousLocation()) <<", " << std::get<1>(it->getPreviousLocation()) << "]");
    prevDistanceToProducer = calculateDistanceToProducer(it->getPreviousLocation(), producerLoc); 
    distanceToProducer = calculateDistanceToProducer(it->getCurrentLocation(), producerLoc);
    if(distanceToProducer <= prevDistanceToProducer){ //node is getting closer to producer
      if(distanceToProducer <= currentNode.second){ //neighbor is more closer than current node
          goodRelays.push_back({it->getId(), distanceToProducer});
          // selectedNeighbor.first = it->getId();
          // selectedNeighbor.second = distanceToProducer;
      }
    }
  }

  //Check if potencial relays set is empty?
  if(goodRelays.empty()){
    return relays;
  }
  //Sort all potencial relays by shortest distance
  std::sort(goodRelays.begin(), goodRelays.end(), [](auto &left, auto &right) { return left.second < right.second; });

  //Select the best N neighbors
  std::vector<std::pair<uint32_t, double>>::iterator it;
  for (it = goodRelays.begin(); it != goodRelays.end(); it++)
  {
    NFD_LOG_DEBUG("Relay " << it->first << " chosen.");
    relays.push_back(it->first);
    if(relays.size() == m_nRelays) break;
  }
  
  return relays;
}

void
BestNeighborStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
    NFD_LOG_DEBUG(interest << " received");
    
    const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
    const fib::NextHopList& nexthops = fibEntry.getNextHops();
    NFD_LOG_DEBUG("nexthops size= " << nexthops.size());

    //get the first nexthop and its face
    const fib::NextHop nextHop = nexthops.front();
    Face& outFace = nextHop.getFace();
    NFD_LOG_DEBUG("outFace= " << outFace.getId());

    if ((outFace.getId() == ingress.face.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(ingress.face, interest, outFace)) {
        return;
    }

    //if outface equals to appFace (P2P), then send interest out
    if(outFace.getLinkType() == ndn::nfd::LINK_TYPE_POINT_TO_POINT){
        this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
        NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
        return;
    }
    //otherwise continue procedure of selecting best neighbor
    //current node
    auto node = ns3::NodeList::GetNode(Simulator::GetContext());

    //get selected neighbor tag and if exists continue
    //std::shared_ptr<ndn::lp::SelectedNeighborTag> selNeiTag = interest.getTag<ndn::lp::SelectedNeighborTag>();
    // if(selNeiTag != nullptr){
    //     uint64_t sNeighbor = selNeiTag->get();
    //     if(node->GetId() != sNeighbor){
    //         //(drop) this interest NOT counts as TimeoutInterest RateTrace
    //         NFD_LOG_DEBUG("Node " << node->GetId() << " not chosen as best neighbor");
    //         this->getNit().setRejectInterest(true);
    //         this->rejectPendingInterest(pitEntry);
    //         return;
    //     }
    // }
    std::shared_ptr<ndn::lp::RelayTag> relayTag = interest.getTag<ndn::lp::RelayTag>();
    if(relayTag != nullptr){
      std::vector<uint32_t> r = relayTag->getRelays();
      //Check if Relays Set contains current Node Id
      if(std::find(r.begin(), r.end(), node->GetId()) == r.end()) {
          /* r does not contain nodeId */
          //delete InRecord from WAVE face, if applied
          NFD_LOG_DEBUG("BestNeighborStrategy nrInRecords=" << pitEntry->getInRecords().size());
          if(pitEntry->getInRecords().size() > 1) {
          //   for (auto it = pitEntry->getInRecords().begin(); it != pitEntry->getInRecords().end(); ++it) {
          //     if(it->getFace().getLinkType() != ndn::nfd::LINK_TYPE_POINT_TO_POINT){
          //       NFD_LOG_DEBUG("BestNeighborStrategy delete face=" << it->getFace().getId() << " from InRecords.");
          //       pitEntry->deleteInRecord(it->getFace());
          //       break;
          //     }
          //   }
          // } else {
            //(drop) this interest NOT counts as TimeoutInterest RateTrace
            //this->rejectPendingInterest(pitEntry);
            //this->getNit().setRejectInterest(true);
          }
          return;
      }
    }

    //std::pair<uint32_t, double> selectedNeighbor = performSelectionOfBestNeighbor(interest);
    std::vector<uint32_t> selectedRelays = performSelectionOfBestNeighbor(interest);
    if(selectedRelays.empty()){
      //(drop)  this interest counts as TimeoutInterest RateTrace
      // this->rejectPendingInterest(pitEntry);
      NFD_LOG_DEBUG("Relays Set is Empty -> Current node is the closest to producer");
      return;
    }

    //set selected neighbor in interest and send it
    interest.setTag<ndn::lp::RelayTag>(std::make_shared<ndn::lp::RelayTag>(selectedRelays));
    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);

    NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
}

} // namespace fw
} // namespace nfd
