#include "my-multicast-strategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"

#include <ndn-cxx/lp/tags.hpp>
#include <cmath>
#include <string.h>

class Point
{
private:
  double x, y;
public:
  Point(double _x, double _y): x(_x), y(_y) {};
  
  double getX(){ return x;}
  double getY(){ return y;}
};

using namespace ns3;

namespace nfd {
namespace fw {

//NFD_REGISTER_STRATEGY(MulticastStrategy);

NFD_LOG_INIT(MyMulticastStrategy);

const time::milliseconds MyMulticastStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds MyMulticastStrategy::RETX_SUPPRESSION_MAX(250);

MyMulticastStrategy::MyMulticastStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , ProcessNackTraits(this)
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                      RETX_SUPPRESSION_MAX)
          
{
  ParsedInstanceName parsed = parseInstanceName(name);
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
MyMulticastStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/my-multicast/%FD%01");
  return strategyName;
}

double areaTriangle(Point p1, Point p2, Point p3){
  return abs(
            p1.getX() * (p2.getY() - p3.getY()) + 
            p2.getX() * (p3.getY() - p1.getY()) + 
            p3.getX() * (p1.getY() - p2.getY())
            ) / 2;
}
bool
isInsideForwardingZone(std::vector<Point> fZone, Point cNode){
  //divide the forwarging zone with the point of current node in triangles and calculate the area of each triangle
  //area of triangle = absolute value of Ax(By - Cy) + Bx(Cy - Ay) + Cx(Ay - By) divided by 2.

  //calculate area of forwarding zone
  double totalArea = round((areaTriangle(fZone[0], fZone[1], fZone[2]) + // (P1, P2, P3) + (P1, P4, P2)
                    areaTriangle(fZone[0], fZone[3], fZone[2])) * 100
                    ) / 100;
  //std::cout << "Total Area = " << totalArea << std::endl;

  //calculate the sum of all new created triangles area
  double sumArea = round((areaTriangle(fZone[0], cNode, fZone[1]) +  // (P1, X, P2)
                  areaTriangle(fZone[1], cNode, fZone[2]) + // (P2, X, P3)
                  areaTriangle(fZone[2], cNode, fZone[3]) + // (P3, X, P4)
                  areaTriangle(fZone[3], cNode, fZone[0])) * 100 // (P4, X, P1)
                ) / 100;

  //std::cout << "Sum Area = " << sumArea << std::endl;

  if(sumArea <= totalArea){
    std::cout << "Inside Forwarding Zone" << std::endl;
    return true;
  }
  return false;

}
std::vector<Point>
defForwardingZone(Point p1, Point p2, int width = 2){
  //calculate vector between p1 and p2
  Point V(p2.getX() - p1.getX(), p2.getY() - p1.getY());

  //calculate a perpendicular to vector previously created
  Point P(V.getY(), -V.getX());

  //normalize perpendicular vector and apply half of width
  double pLength = sqrt(P.getX() * P.getX() + P.getY() * P.getY());
  double hWidth = width / 2;
  Point N(hWidth * P.getX() / pLength, hWidth * P.getY() / pLength);

  //calculate all 4 necessary points
  Point r1(p2.getX() - N.getX(), p2.getY()  - N.getY());
  Point r2(p2.getX() + N.getX(), p2.getY()  + N.getY());    
  Point r3(p1.getX() + N.getX(), p1.getY() + N.getY());
  Point r4(p1.getX() - N.getX(), p1.getY() - N.getY());
  std::vector<Point> points{r1,r2,r3,r4};

  for (size_t i = 0; i < points.size(); i++)
    std::cout << "Point[" << i << "] = " << points[i].getX() << ',' << points[i].getY() << std::endl;

  return points;
}

Point
getProducerLocation(const Interest& interest){
  Name interestName = interest.getName().getPrefix(2);  
  Point p(std::stod(interestName.get(0).toUri()), std::stod(interestName.get(1).toUri()));
  return p;
}

Point
getConsumerLocation(const Interest& interest){
  std::shared_ptr<ndn::lp::GeoTag> consumerTag = interest.getTag<ndn::lp::GeoTag>();
  Point p(std::get<0>(consumerTag->getPos()), std::get<1>(consumerTag->getPos()));
  return p;
}

void
MyMulticastStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG(interest << " received");

  auto node = ns3::NodeList::GetNode(Simulator::GetContext());
  Vector position = node->GetObject<MobilityModel> ()->GetPosition();

  Point currentPosition{ position.x, position.y};
  Point consumerLocation = getConsumerLocation(interest);
  Point producerLocation = getProducerLocation(interest);
  std::cout << "Current Location: " << currentPosition.getX() <<","<< currentPosition.getY() << std::endl;
  std::cout << "Consumer Location: " << consumerLocation.getX() <<","<< consumerLocation.getY() << std::endl;
  std::cout << "Producer Location: " << producerLocation.getX() <<","<< producerLocation.getY() << std::endl;
  
  std::vector<Point> forwardingZone = defForwardingZone(consumerLocation, producerLocation);

  if(isInsideForwardingZone(forwardingZone, currentPosition)) {
    //Node is inside Forwarding zone
    const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
    const fib::NextHopList& nexthops = fibEntry.getNextHops();

    int nEligibleNextHops = 0;
    bool isSuppressed = false;
    NFD_LOG_DEBUG("nexthops size= " << nexthops.size());
    std::cout << "nexthops size= " << nexthops.size() << std::endl;
    for (const auto& nexthop : nexthops) {
      Face& outFace = nexthop.getFace();

      RetxSuppressionResult suppressResult = m_retxSuppression.decidePerUpstream(*pitEntry, outFace);
      if (suppressResult == RetxSuppressionResult::SUPPRESS) {
        NFD_LOG_DEBUG(interest << " from=" << ingress << " to=" << outFace.getId() << " suppressed");
        isSuppressed = true;
        continue;
      }
      
      NFD_LOG_DEBUG("outFace link type=" << outFace.getLinkType());
      if ((outFace.getId() == ingress.face.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
          wouldViolateScope(ingress.face, interest, outFace)) {
        continue;
      }

      this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
      NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
      
      if (suppressResult == RetxSuppressionResult::FORWARD) {
        m_retxSuppression.incrementIntervalForOutRecord(*pitEntry->getOutRecord(outFace));
      }
      ++nEligibleNextHops;
    }
  } else {
    std::cout << "Node at position [" << currentPosition.getX() << "," << currentPosition.getY() << "] is not inside forwarding zone" << std::endl;
    this->rejectPendingInterest(pitEntry);
  }

  // if (nEligibleNextHops == 0 && !isSuppressed) {
  //   NFD_LOG_DEBUG(interest << " from=" << ingress << " noNextHop");
  //   NFD_LOG_DEBUG("send Nack");
  //   lp::NackHeader nackHeader;
  //   nackHeader.setReason(lp::NackReason::NO_ROUTE);
  //   this->sendNack(pitEntry, ingress, nackHeader);

  //   this->rejectPendingInterest(pitEntry);
  // }
}
//Como os links são ad-hoc, o pacote nack é deitado fora
void
MyMulticastStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                                    const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("afterReceiveNack");
  this->processNack(ingress.face, nack, pitEntry);
}

} // namespace fw
} // namespace nfd
