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

#include "neighbor-table.hpp"

#include <ndn-cxx/util/concepts.hpp>
#include "common/global.hpp"


namespace nfd {
namespace neighbor_table {
NeighborTable::NeighborTable()
{
}

Entry *
NeighborTable::insert(std::pair<uint32_t, std::tuple<double, double, double>> neighbor, std::tuple<double, double, double> lastNeighborPosition)
{
  uint32_t nodeId = neighbor.first;
  std::tuple<double, double, double> nodeLocation = neighbor.second;
  
  const_iterator it;
  bool isNewEntry = false;
  std::tie(it, isNewEntry) = m_table.emplace(nodeId, std::make_pair(nodeLocation, lastNeighborPosition));
  Entry& entry = const_cast<Entry&>(*it);
  
  if(!isNewEntry){
    entry.updateLocation(std::make_pair(nodeLocation, lastNeighborPosition));
  }
  m_lastUpdated = ndn::time::steady_clock::now();

  return &entry;
}
} // namespace fib
} // namespace nfd
