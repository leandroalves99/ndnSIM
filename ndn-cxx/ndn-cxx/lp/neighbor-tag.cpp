#include "ndn-cxx/lp/neighbor-tag.hpp"
#include "ndn-cxx/lp/tlv.hpp"
#include <iostream>

namespace ndn {
namespace lp {

NeighborTag::NeighborTag(const Block& block)
{
  wireDecode(block);
}

template<encoding::Tag TAG>
size_t
NeighborTag::wireEncode(EncodingImpl<TAG>& encoder) const
{
    
  size_t length = 0;
  length += prependNonNegativeIntegerBlock(encoder, tlv::NeighborTagId, m_neighbor.first);
  length += prependDoubleBlock(encoder, tlv::NeighborTagPos, std::get<2>(m_neighbor.second));
  length += prependDoubleBlock(encoder, tlv::NeighborTagPos, std::get<1>(m_neighbor.second));
  length += prependDoubleBlock(encoder, tlv::NeighborTagPos, std::get<0>(m_neighbor.second));
  length += encoder.prependVarNumber(length);
  length += encoder.prependVarNumber(tlv::NeighborTag);
  return length;
}

template size_t
NeighborTag::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& encoder) const;

template size_t
NeighborTag::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& encoder) const;

const Block&
NeighborTag::wireEncode() const
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
NeighborTag::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::NeighborTag) {
    NDN_THROW(ndn::tlv::Error("expecting NeighborTag block"));
  }

  m_wire = wire;
  m_wire.parse();


  if (m_wire.elements().size() < 4 ||
      m_wire.elements()[0].type() != tlv::NeighborTagPos ||
      m_wire.elements()[1].type() != tlv::NeighborTagPos ||
      m_wire.elements()[2].type() != tlv::NeighborTagPos ||
      m_wire.elements()[3].type() != tlv::NeighborTagId) {
    NDN_THROW(ndn::tlv::Error("Unexpected input while decoding NeighborTag"));
  }
  
  m_neighbor = {
      encoding::readNonNegativeInteger(m_wire.elements()[3]),
      {
          encoding::readDouble(m_wire.elements()[0]),
          encoding::readDouble(m_wire.elements()[1]),
          encoding::readDouble(m_wire.elements()[2])
      }
  };
    
}

} // namespace lp
} // namespace ndn
