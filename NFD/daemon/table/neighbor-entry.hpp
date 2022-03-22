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

#ifndef NFD_DAEMON_TABLE_NEIGHBOR_ENTRY_HPP
#define NFD_DAEMON_TABLE_NEIGHBOR_ENTRY_HPP

#include <ndn-cxx/util/scheduler.hpp>
#include <boost/utility.hpp>
#include <iostream>
#include <functional>
#include <set>

namespace nfd {

namespace neighbor_table {

class NeighborTable;

class Entry
{
public:
  Entry( uint32_t id, std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>> location);  //<new location, old location>

  uint32_t
  getId() const
  {
    return m_id;
  }

  std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>
  getLocations() const
  {
    return m_location;
  }

  std::tuple<double, double, double>
  getCurrentLocation() const
  {
    return m_location.first;
  }

  std::tuple<double, double, double>
  getPreviousLocation() const
  {
    return m_location.second;
  }

  void
  updateLocation(std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>> newLoc)
  {
    m_location = newLoc;
  }
  
public:
  /** \brief Expiry timer
   *
   *  This timer is used in forwarding pipelines to delete the entry
   */
  ndn::scheduler::EventId expiryTimer;
  
private:
  /** \brief Neighbor ID
   *
   *  This id is used to identify the neighbor
   */
  int m_id;
  /** \brief Neighbor Locations
   *
   *  This pair corresponds to current and previous location of the neighbor
   */
  std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>> m_location;

  
};

bool
operator<(const Entry& entry, const uint32_t& queryId);

bool
operator<(const uint32_t& queryId, const Entry& entry);

bool
operator<(const Entry& lhs, const Entry& rhs);

using Table = std::set<Entry, std::less<>>;

inline bool
operator<(Table::const_iterator lhs, Table::const_iterator rhs)
{
  return *lhs < *rhs;
}

} // namespace nit
} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_ENTRY_HPP
