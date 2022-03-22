/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_TABLE_NEIGHBOR_HPP
#define NFD_DAEMON_TABLE_NEIGHBOR_HPP


#include <iostream>
#include <vector>
#include <memory>
#include <cstdio>
#include <fstream>
#include <cassert>
#include <functional>
#include <tuple>

#include "neighbor-entry.hpp"
#include <boost/range/adaptor/transformed.hpp>
#include <ndn-cxx/util/time.hpp>

using namespace std;
namespace nfd {

namespace neighbor_table {

/** \brief Represents the Neighbor Information Table (NIT)
 */
class NeighborTable : boost::noncopyable
{
public:
  explicit
  NeighborTable();

  /** \brief inserts a Node Information
   */
  Entry *
  insert(std::pair<uint32_t, std::tuple<double, double, double>> neighbor, std::tuple<double, double, double> lastNeighborPosition);

  /** \brief deletes all entries
   */
  void
  clear(){
    m_table.clear();
  }

  /** \brief Delete an entry
   */
  void
  eraseEntry(Entry& entry){
    m_table.erase(entry);
  }
  
  bool
  getRejectInterest(){
    return m_rejectInterest;
  }

  void setRejectInterest(bool b){
    m_rejectInterest = b;
  }

  size_t
  size() const
  {
    return m_table.size();
  }

public: // enumeration
  using const_iterator = Table::const_iterator;

  const_iterator
  begin() const
  {
    return m_table.begin();
  }

  const_iterator
  end() const
  {
    return m_table.end();
  }

  ndn::time::steady_clock::TimePoint
  getLastUpdated() const
  {
    return m_lastUpdated;
  }

private:
  Table m_table;
  bool m_rejectInterest = false;  // check if pending interest is reject or not to delete PIT entry
  ndn::time::steady_clock::TimePoint m_lastUpdated = ndn::time::steady_clock::TimePoint::min();   //last time when table was updated
  
};

} // namespace nit
using neighbor_table::NeighborTable;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_NEIGHBOR_HPP
