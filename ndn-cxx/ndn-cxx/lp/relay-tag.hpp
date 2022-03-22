#ifndef NDN_CXX_LP_RELAY_TAG_HPP
#define NDN_CXX_LP_RELAY_TAG_HPP

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/tag.hpp"

namespace ndn {
namespace lp {

/**
 * \brief represents a GeoTag header field
 */
class RelayTag : public Tag
{
public:
  static constexpr int
  getTypeId() noexcept
  {
    return 0x60000004;
  }

  RelayTag() = default;

  explicit
  RelayTag(std::vector<uint32_t> relays)
    : m_relays(relays)
  {
  }

  explicit
  RelayTag(const Block& block);

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
  std::vector<uint32_t>
  getRelays() const
  {
    return m_relays;
  }

  /**
   * \brief set neighbor - id,location
   */
  RelayTag*
  setRelays(std::vector<uint32_t> r)
  {
    m_relays = r;
    return this;
  }

private:
  std::vector<uint32_t> m_relays;
  mutable Block m_wire;
};

} // namespace lp
} // namespace ndn

#endif // NDN_CXX_LP_RELAY_TAG_HPP
