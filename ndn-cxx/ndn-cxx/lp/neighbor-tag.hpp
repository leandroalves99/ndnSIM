#ifndef NDN_CXX_LP_NEIGHBOR_TAG_HPP
#define NDN_CXX_LP_NEIGHBOR_TAG_HPP

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/tag.hpp"

namespace ndn {
namespace lp {

/**
 * \brief represents a GeoTag header field
 */
class NeighborTag : public Tag
{
public:
  static constexpr int
  getTypeId() noexcept
  {
    return 0x60000002;
  }

  NeighborTag() = default;

  explicit
  NeighborTag(std::pair<uint32_t, std::tuple<double, double, double>> neighbor)
    : m_neighbor(neighbor)
  {
  }

  explicit
  NeighborTag(const Block& block);

  /**
   * \brief prepend GeoTag to encoder
   */
  template<encoding::Tag TAG>
  size_t
  wireEncode(EncodingImpl<TAG>& encoder) const;

  /**
   * \brief encode GeoTag into wire format
   */
  const Block&
  wireEncode() const;

  /**
   * \brief get GeoTag from wire format
   */
  void
  wireDecode(const Block& wire);

public: // get & set NeighborTag
  /**
   * \return neighbor pair of NeighborTag
   */
  std::pair<uint32_t, std::tuple<double, double, double>>
  getNeighbor() const
  {
    return m_neighbor;
  }

  /**
   * \brief set neighbor - id,location
   */
  NeighborTag*
  setNeighbor(std::pair<uint32_t, std::tuple<double, double, double>> neighbor)
  {
    m_neighbor = neighbor;
    return this;
  }

private:
  std::pair<uint32_t, std::tuple<double, double, double>> m_neighbor;
  mutable Block m_wire;
};

} // namespace lp
} // namespace ndn

#endif // NDN_CXX_LP_NEIGHBORTAG_HPP
