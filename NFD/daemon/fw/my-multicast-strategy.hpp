#ifndef NFD_DAEMON_FW_MY_MULTICAST_STRATEGY_HPP
#define NFD_DAEMON_FW_MY_MULTICAST_STRATEGY_HPP

#include "strategy.hpp"
#include "process-nack-traits.hpp"
#include "retx-suppression-exponential.hpp"

namespace nfd {
namespace fw {

/** \brief a forwarding strategy that forwards Interest to all FIB nexthops
 *
 *  \note This strategy is not EndpointId-aware.
 */
class MyMulticastStrategy : public Strategy
                        , public ProcessNackTraits<MyMulticastStrategy>
{
public:
  explicit
  MyMulticastStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  static const Name&
  getStrategyName();

  void
  afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  void
  afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry) override;

private:
  friend ProcessNackTraits<MyMulticastStrategy>;
  RetxSuppressionExponential m_retxSuppression;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_MULTICAST_STRATEGY_HPP
