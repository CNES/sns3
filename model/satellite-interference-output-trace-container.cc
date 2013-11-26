/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Frans Laakso <frans.laakso@magister.fi>
 */
#include "satellite-interference-output-trace-container.h"
#include "ns3/satellite-env-variables.h"

NS_LOG_COMPONENT_DEFINE ("SatInterferenceOutputTraceContainer");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatInterferenceOutputTraceContainer);

TypeId 
SatInterferenceOutputTraceContainer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatInterferenceOutputTraceContainer")
    .SetParent<SatBaseTraceContainer> ();
  return tid;
}

SatInterferenceOutputTraceContainer::SatInterferenceOutputTraceContainer () :
  m_index (0),
  m_currentWorkingDirectory ("")
{
  NS_LOG_FUNCTION (this);

  Ptr<SatEnvVariables> envVariables = CreateObject<SatEnvVariables> ();
  m_currentWorkingDirectory = envVariables->GetCurrentWorkingDirectory ();
}

SatInterferenceOutputTraceContainer::~SatInterferenceOutputTraceContainer ()
{
  NS_LOG_FUNCTION (this);

  Reset ();
}

void
SatInterferenceOutputTraceContainer::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Reset ();

  SatBaseTraceContainer::DoDispose();
}

void
SatInterferenceOutputTraceContainer::Reset ()
{
  NS_LOG_FUNCTION (this);

  if ( !m_container.empty() )
    {
      m_container.clear();
    }
  m_index = 0;
  m_currentWorkingDirectory = "";
}

void
SatInterferenceOutputTraceContainer::AddNode (key_t key)
{
  NS_LOG_FUNCTION (this);

  std::stringstream filename;

  filename << m_currentWorkingDirectory << "/data/interference_trace/output/nodeId_" << m_index << "_channelType_" << key.second;

  std::pair <container_t::iterator, bool> result = m_container.insert (std::make_pair(key, CreateObject<SatOutputFileStreamDoubleContainer> (filename.str().c_str(), std::ios::out, SatBaseTraceContainer::INTF_TRACE_DEFAULT_NUMBER_OF_COLUMNS)));

  if (result.second == false)
    {
      NS_FATAL_ERROR ("SatInterferenceOutputTraceContainer::AddNode failed");
    }

  NS_LOG_INFO ("SatInterferenceOutputTraceContainer::AddNode: Added node with ID " << m_index);

  m_index++;
}

Ptr<SatOutputFileStreamDoubleContainer>
SatInterferenceOutputTraceContainer::FindNode (key_t key)
{
  NS_LOG_FUNCTION (this);

  return m_container.at (key);
}

void
SatInterferenceOutputTraceContainer::WriteToFile (key_t key)
{
  NS_LOG_FUNCTION (this);

  FindNode (key)->WriteContainerToFile ();
}

void
SatInterferenceOutputTraceContainer::AddToContainer (key_t key, std::vector<double> newItem)
{
  NS_LOG_FUNCTION (this);

  if ( newItem.size() != SatBaseTraceContainer::INTF_TRACE_DEFAULT_NUMBER_OF_COLUMNS)
    {
      NS_FATAL_ERROR ("SatInterferenceOutputTraceContainer::AddToContainer - Incorrect vector size");
    }

  FindNode (key)->AddToContainer (newItem);
}

} // namespace ns3
