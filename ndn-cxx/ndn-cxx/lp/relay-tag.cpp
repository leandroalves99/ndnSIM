#include "ndn-cxx/lp/relay-tag.hpp"
#include "ndn-cxx/lp/tlv.hpp"
#include <iostream>

namespace ndn {
namespace lp {

RelayTag::RelayTag(const Block& block)
{
  wireDecode(block);
}

template<encoding::Tag TAG>
size_t
RelayTag::wireEncode(EncodingImpl<TAG>& encoder) const
{
  if (m_relays.size() == 0) BOOST_THROW_EXCEPTION(ndn::tlv::Error("Relays Set must not be empty"));

  size_t length = 0;
  for (size_t i = 0; i < m_relays.size(); i++)
  {
    length += prependNonNegativeIntegerBlock(encoder, tlv::RelayTagId, m_relays.at(i));
  }
  length += encoder.prependVarNumber(length);
  length += encoder.prependVarNumber(tlv::RelayTag);
  return length;
}

template size_t
RelayTag::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& encoder) const;

template size_t
RelayTag::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& encoder) const;

const Block&
RelayTag::wireEncode() const
{
  if (m_wire.hasWire()) {
    return m_wire;
  }

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();

  return m_wire;
}

void
RelayTag::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::RelayTag) {
    NDN_THROW(ndn::tlv::Error("expecting RelayTag block"));
  }

  m_wire = wire;
  m_wire.parse();

  if (m_wire.elements().size() == 0) {
    NDN_THROW(ndn::tlv::Error("Unexpected input while decoding RelayTag"));
  }

  std::vector<uint32_t> r;
  Block::element_const_iterator it;
  for (it = m_wire.elements_begin(); it != m_wire.elements_end(); it++)
  {

    r.push_back(static_cast<uint32_t>(readNonNegativeInteger(*it)));
  }
  m_relays = r;

}

} // namespace lp
} // namespace ndn
