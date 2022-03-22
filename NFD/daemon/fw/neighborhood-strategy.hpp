#ifndef NFD_DAEMON_FW_NEIGHBORHOOD_STRATEGY_HPP
#define NFD_DAEMON_FW_NEIGHBORHOOD_STRATEGY_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

/** \brief a forwarding strategy that forwards Interest to all FIB nexthops
 *
 *  \note This strategy is not EndpointId-aware.
 */
class NeighborhoodStrategy : public Strategy
{
public:
  explicit
  NeighborhoodStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

  void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveData(const shared_ptr<pit::Entry>& pitEntry,
                   const FaceEndpoint& ingress, const Data& data);
  void
  onNitEntryExpires(neighbor_table::Entry *entry);

private:
  time::milliseconds entryLifetime; ///< @brief time of NIT entry lifetime
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_NEIGHBORHOOD_STRATEGY_HPP
