#ifndef NFD_DAEMON_FW_BEST_NEIGHBOR_STRATEGY_HPP
#define NFD_DAEMON_FW_FW_BEST_NEIGHBOR_STRATEGY_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

/** \brief a forwarding strategy that forwards Interest to all FIB nexthops
 *
 *  \note This strategy is not EndpointId-aware.
 */
class BestNeighborStrategy : public Strategy
{
public:
  explicit
  BestNeighborStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

  void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

private:
  std::tuple<double, double, double> 
  getProducerLocation(const Interest& interest);
    
  std::vector<uint32_t>
  performSelectionOfBestNeighbor(const Interest& interest);

private:
  int m_nRelays;  // number of relays
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_BEST_NEIGHBOR_STRATEGY_HPP
