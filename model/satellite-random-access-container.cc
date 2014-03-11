/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Magister Solutions Ltd.
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
#include "satellite-random-access-container.h"

NS_LOG_COMPONENT_DEFINE ("SatRandomAccess");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatRandomAccess);

TypeId 
SatRandomAccess::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatRandomAccess")
    .SetParent<Object> ();
  return tid;
}

SatRandomAccess::SatRandomAccess () :
  m_uniformRandomVariable (),
  m_randomAccessModel (RA_OFF),
  m_randomAccessConf (),
  m_numOfAllocationChannels (),

  /// CRDSA variables
  m_crdsaNewData (true)
{
  NS_LOG_FUNCTION (this);

  NS_FATAL_ERROR ("SatRandomAccess::SatRandomAccess - Constructor not in use");
}

SatRandomAccess::SatRandomAccess (Ptr<SatRandomAccessConf> randomAccessConf, RandomAccessModel_t randomAccessModel) :
  m_uniformRandomVariable (),
  m_randomAccessModel (randomAccessModel),
  m_randomAccessConf (randomAccessConf),
  m_numOfAllocationChannels (randomAccessConf->GetNumOfAllocationChannels ()),

  /// CRDSA variables
  m_crdsaNewData (true)
{
  NS_LOG_FUNCTION (this);

  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();

  if (m_randomAccessConf == NULL)
    {
      NS_FATAL_ERROR ("SatRandomAccess::SatRandomAccess - Configuration object is NULL");
    }

  SetRandomAccessModel (randomAccessModel);
}

SatRandomAccess::~SatRandomAccess ()
{
  NS_LOG_FUNCTION (this);

  m_uniformRandomVariable = NULL;
  m_randomAccessConf = NULL;

  if (!m_isDamaAvailableCb.IsNull ())
    {
      m_isDamaAvailableCb.Nullify ();
    }

  if (!m_areBuffersEmptyCb.IsNull ())
    {
      m_areBuffersEmptyCb.Nullify ();
    }

  if (!m_numOfCandidatePacketsCb.IsNull ())
    {
      m_numOfCandidatePacketsCb.Nullify ();
    }
  m_crdsaAllocationChannels.clear ();
  m_slottedAlohaAllocationChannels.clear ();
}

void
SatRandomAccess::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_uniformRandomVariable = NULL;
  m_randomAccessConf = NULL;

  if (!m_isDamaAvailableCb.IsNull ())
    {
      m_isDamaAvailableCb.Nullify ();
    }

  if (!m_areBuffersEmptyCb.IsNull ())
    {
      m_areBuffersEmptyCb.Nullify ();
    }

  if (!m_numOfCandidatePacketsCb.IsNull ())
    {
      m_numOfCandidatePacketsCb.Nullify ();
    }
  m_crdsaAllocationChannels.clear ();
  m_slottedAlohaAllocationChannels.clear ();
}

///---------------------------------------
/// General random access related methods
///---------------------------------------

void
SatRandomAccess::SetRandomAccessModel (RandomAccessModel_t randomAccessModel)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("SatRandomAccess::SetRandomAccessModel - Setting Random Access model to: " << randomAccessModel);

  m_randomAccessModel = randomAccessModel;

  NS_LOG_INFO ("SatRandomAccess::SetRandomAccessModel - Random Access model updated");
}

void
SatRandomAccess::AddCrdsaAllocationChannel (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  std::pair<std::set<uint32_t>::iterator,bool> result = m_crdsaAllocationChannels.insert (allocationChannel);

  if (!result.second)
    {
      NS_FATAL_ERROR ("SatRandomAccess::AddCrdsaAllocationChannel - insert failed, this allocation channel is already set");
    }
}

void
SatRandomAccess::AddSlottedAlohaAllocationChannel (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  std::pair<std::set<uint32_t>::iterator,bool> result = m_slottedAlohaAllocationChannels.insert (allocationChannel);

  if (!result.second)
    {
      NS_FATAL_ERROR ("SatRandomAccess::AddSlottedAlohaAllocationChannel - insert failed, this allocation channel is already set");
    }
}

bool
SatRandomAccess::IsCrdsaAllocationChannel (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  std::set<uint32_t>::iterator iter;
  iter = m_crdsaAllocationChannels.find (allocationChannel);

  if (iter == m_crdsaAllocationChannels.end())
    {
      return false;
    }
  return true;
}

bool
SatRandomAccess::IsSlottedAlohaAllocationChannel (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  std::set<uint32_t>::iterator iter;
  iter = m_slottedAlohaAllocationChannels.find (allocationChannel);

  if (iter == m_slottedAlohaAllocationChannels.end())
    {
      return false;
    }
  return true;
}

SatRandomAccess::RandomAccessTxOpportunities_s
SatRandomAccess::DoRandomAccess (uint32_t allocationChannel, RandomAccessTriggerType_t triggerType)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("SatRandomAccess::DoRandomAccess, AC: " << allocationChannel << " trigger: " << triggerType);

  /// return variable initialization
  RandomAccessTxOpportunities_s txOpportunities;

  txOpportunities.txOpportunityType = SatRandomAccess::RA_DO_NOTHING;

  NS_LOG_INFO ("------------------------------------");
  NS_LOG_INFO ("------ Starting Random Access ------");
  NS_LOG_INFO ("------------------------------------");

  /// Do CRDSA
  if (m_randomAccessModel == RA_CRDSA && triggerType == RA_CRDSA_TRIGGER)
    {
      NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - Only CRDSA enabled && CRDSA trigger, evaluating CRDSA");

      txOpportunities = DoCrdsa (allocationChannel);
    }
  /// Do Slotted ALOHA
  else if (m_randomAccessModel == RA_SLOTTED_ALOHA && triggerType == RA_SLOTTED_ALOHA_TRIGGER)
    {
      NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - Only SA enabled, evaluating Slotted ALOHA");

      txOpportunities = DoSlottedAloha ();
    }
  /// Frame start is a known trigger for CRDSA
  /// If SA is triggered by a buffer arrival at frame start, both SA and CRDSA will be evaluated.
  /// TODO: Although the possibility is remote, this approach is not optimal and should be improved in the future

  /// When CRDSA is triggered, if the following conditions are fulfilled, SA shall be used instead of CRDSA
  /// 1) CRDSA back off probability is higher than the parameterized value
  /// 2) CRDSA back off is in effect
  else if (m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - All RA models enabled");

      if (triggerType == RA_SLOTTED_ALOHA_TRIGGER)
        {
          NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - Evaluating Slotted ALOHA");
          txOpportunities = DoSlottedAloha ();
        }
      else
        {
          NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - Checking CRDSA backoff & backoff probability");

          if (CrdsaHasBackoffTimePassed (allocationChannel) && !IsCrdsaBackoffProbabilityTooHigh (allocationChannel))
            {
              NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - Low CRDSA backoff value AND CRDSA is free, evaluating CRDSA");
              txOpportunities = DoCrdsa (allocationChannel);
            }
          else
            {
              NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - High CRDSA backoff value OR CRDSA is not free, evaluating Slotted ALOHA");
              txOpportunities = DoSlottedAloha ();

              CrdsaReduceIdleBlocksForAllAllocationChannels ();
            }
        }
    }

  /// For debugging purposes
  /// TODO: comment out this code at later stage
  if (txOpportunities.txOpportunityType == SatRandomAccess::RA_CRDSA_TX_OPPORTUNITY)
    {
      for (uint32_t i = 0; i < txOpportunities.crdsaTxOpportunities.size (); i++)
        {
          std::set<uint32_t>::iterator iterSet;
          for (iterSet = txOpportunities.crdsaTxOpportunities[i].begin(); iterSet != txOpportunities.crdsaTxOpportunities[i].end(); iterSet++)
            {
              NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - CRDSA transmission opportunity for unique packet: " << i << " at slot: " << (*iterSet));
            }
        }
    }
  else if (txOpportunities.txOpportunityType == SatRandomAccess::RA_SLOTTED_ALOHA_TX_OPPORTUNITY)
    {
      NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - SA minimum time to wait: " << txOpportunities.slottedAlohaTxOpportunity << " milliseconds");
    }
  else if (txOpportunities.txOpportunityType == SatRandomAccess::RA_DO_NOTHING)
    {
      NS_LOG_INFO ("SatRandomAccess::DoRandomAccess - No Tx opportunity");
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::DoRandomAccess - Invalid result type");
    }

  NS_LOG_INFO ("------------------------------------");
  NS_LOG_INFO ("------ Random Access FINISHED ------");
  NS_LOG_INFO ("------------------------------------");

  txOpportunities.allocationChannel = allocationChannel;

  return txOpportunities;
}

void
SatRandomAccess::PrintVariables ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Simulation time: " << Now ().GetSeconds () << " seconds");
  NS_LOG_INFO ("Num of allocation channels: " << m_numOfAllocationChannels);
  NS_LOG_INFO ("New data status: " << m_crdsaNewData);

  NS_LOG_INFO ("---------------");

  for (uint32_t index = 0; index < m_numOfAllocationChannels; index++)
    {
      NS_LOG_INFO ("ALLOCATION CHANNEL: " << index);
      NS_LOG_INFO ("Backoff release at: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaBackoffReleaseTime () << " seconds");
      NS_LOG_INFO ("Backoff time: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaBackoffTime () << " milliseconds");
      NS_LOG_INFO ("Backoff probability: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaBackoffProbability () * 100 << " %");
      NS_LOG_INFO ("Slot randomization: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaNumOfInstances () * m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaMaxUniquePayloadPerBlock () <<
                   " Tx opportunities with range from " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaMinRandomizationValue () <<
                   " to " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaMaxRandomizationValue ());
      NS_LOG_INFO ("Number of unique payloads per block: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaMaxUniquePayloadPerBlock ());
      NS_LOG_INFO ("Number of instances: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaNumOfInstances ());
      NS_LOG_INFO ("Number of consecutive blocks accessed: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaNumOfConsecutiveBlocksUsed () << "/" << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaMaxConsecutiveBlocksAccessed ());
      NS_LOG_INFO ("Number of idle blocks left: " << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaIdleBlocksLeft () << "/" << m_randomAccessConf->GetAllocationChannelConfiguration (index)->GetCrdsaMinIdleBlocks ());
    }
}

///-------------------------------
/// Slotted ALOHA related methods
///-------------------------------

void
SatRandomAccess::SlottedAlohaSetControlRandomizationInterval (double controlRandomizationInterval)
{
  NS_LOG_FUNCTION (this << controlRandomizationInterval);

  if (m_randomAccessModel == RA_SLOTTED_ALOHA || m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      m_randomAccessConf->SetSlottedAlohaControlRandomizationInterval (controlRandomizationInterval);

      m_randomAccessConf->DoSlottedAlohaVariableSanityCheck ();
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::SlottedAlohaSetRandomizationParameters - Wrong random access model in use");
    }

  NS_LOG_INFO ("SatRandomAccess::SlottedAlohaSetRandomizationParameters - new control randomization interval : " << controlRandomizationInterval);
}

SatRandomAccess::RandomAccessTxOpportunities_s
SatRandomAccess::DoSlottedAloha ()
{
  NS_LOG_FUNCTION (this);

  RandomAccessTxOpportunities_s txOpportunity;
  txOpportunity.txOpportunityType = SatRandomAccess::RA_DO_NOTHING;

  NS_LOG_INFO ("---------------------------------------------");
  NS_LOG_INFO ("------ Running Slotted ALOHA algorithm ------");
  NS_LOG_INFO ("---------------------------------------------");
  NS_LOG_INFO ("Slotted ALOHA control randomization interval: " << m_randomAccessConf->GetSlottedAlohaControlRandomizationInterval () << " milliseconds");
  NS_LOG_INFO ("---------------------------------------------");

  NS_LOG_INFO ("SatRandomAccess::DoSlottedAloha - Checking if we have DAMA allocations");

  /// Check if we have known DAMA allocations
  if (!m_isDamaAvailableCb ())
    {
      NS_LOG_INFO ("SatRandomAccess::DoSlottedAloha - No DAMA -> Running Slotted ALOHA");

      /// Randomize Tx opportunity release time
      txOpportunity.slottedAlohaTxOpportunity = SlottedAlohaRandomizeReleaseTime ();
      txOpportunity.txOpportunityType = SatRandomAccess::RA_SLOTTED_ALOHA_TX_OPPORTUNITY;
    }

  NS_LOG_INFO ("----------------------------------------------");
  NS_LOG_INFO ("------ Slotted ALOHA algorithm FINISHED ------");
  NS_LOG_INFO ("----------------------------------------------");

  return txOpportunity;
}

uint32_t
SatRandomAccess::SlottedAlohaRandomizeReleaseTime ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("SatRandomAccess::SlottedAlohaRandomizeReleaseTime - Randomizing the release time...");

  uint32_t releaseTime = m_uniformRandomVariable->GetInteger (0, m_randomAccessConf->GetSlottedAlohaControlRandomizationInterval ());

  NS_LOG_INFO ("SatRandomAccess::SlottedAlohaRandomizeReleaseTime - TX opportunity in the next slot after " << releaseTime << " milliseconds");

  return releaseTime;
}

///-----------------------
/// CRDSA related methods
///-----------------------

void
SatRandomAccess::CrdsaSetLoadControlParameters (uint32_t allocationChannel,
                                                double backoffProbability,
                                                uint32_t backoffTime)
{
  NS_LOG_FUNCTION (this);

  if (m_randomAccessModel == RA_CRDSA || m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaBackoffProbability (backoffProbability);
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaBackoffTime (backoffTime);

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->DoCrdsaVariableSanityCheck ();
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::CrdsaSetLoadControlParameters - Wrong random access model in use");
    }
}

void
SatRandomAccess::CrdsaSetMaximumBackoffProbability (uint32_t allocationChannel,
                                                    double maximumBackoffProbability)
{
  NS_LOG_FUNCTION (this);

  if (m_randomAccessModel == RA_CRDSA || m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaMaximumBackoffProbability (maximumBackoffProbability);

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->DoCrdsaVariableSanityCheck ();
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::CrdsaSetMaximumBackoffProbability - Wrong random access model in use");
    }
}

void
SatRandomAccess::CrdsaSetPayloadBytes (uint32_t allocationChannel,
                                       uint32_t payloadBytes)
{
  NS_LOG_FUNCTION (this);

  if (m_randomAccessModel == RA_CRDSA || m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaPayloadBytes (payloadBytes);

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->DoCrdsaVariableSanityCheck ();
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::CrdsaSetMaximumBackoffProbability - Wrong random access model in use");
    }
}

void
SatRandomAccess::CrdsaSetRandomizationParameters (uint32_t allocationChannel,
                                                  uint32_t minRandomizationValue,
                                                  uint32_t maxRandomizationValue,
                                                  uint32_t numOfInstances)
{
  NS_LOG_FUNCTION (this);

  if (m_randomAccessModel == RA_CRDSA || m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaMinRandomizationValue (minRandomizationValue);
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaMaxRandomizationValue (maxRandomizationValue);
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaNumOfInstances (numOfInstances);

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->DoCrdsaVariableSanityCheck ();
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::CrdsaSetRandomizationParameters - Wrong random access model in use");
    }
}

void
SatRandomAccess::CrdsaSetMaximumDataRateLimitationParameters (uint32_t allocationChannel,
                                                              uint32_t maxUniquePayloadPerBlock,
                                                              uint32_t maxConsecutiveBlocksAccessed,
                                                              uint32_t minIdleBlocks)
{
  NS_LOG_FUNCTION (this);

  if (m_randomAccessModel == RA_CRDSA || m_randomAccessModel == RA_ANY_AVAILABLE)
    {
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaMaxUniquePayloadPerBlock (maxUniquePayloadPerBlock);
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaMaxConsecutiveBlocksAccessed (maxConsecutiveBlocksAccessed);
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaMinIdleBlocks (minIdleBlocks);

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->DoCrdsaVariableSanityCheck ();
    }
  else
    {
      NS_FATAL_ERROR ("SatRandomAccess::CrdsaSetMaximumDataRateLimitationParameters - Wrong random access model in use");
    }
}

bool
SatRandomAccess::IsCrdsaBackoffProbabilityTooHigh (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  bool isBackoffProbabilityTooHigh = false;

  if ((m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaBackoffProbability ()
      >= m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaMaximumBackoffProbability ()))
    {
      isBackoffProbabilityTooHigh = true;
    }

  NS_LOG_INFO ("SatRandomAccess::IsCrdsaBackoffProbabilityTooHigh for allocation channel " << allocationChannel << ": "  << isBackoffProbabilityTooHigh);

  return isBackoffProbabilityTooHigh;
}

bool
SatRandomAccess::CrdsaHasBackoffTimePassed (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  bool hasCrdsaBackoffTimePassed = false;

  if ((Now ().GetSeconds () >= m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaBackoffReleaseTime ()))
    {
      hasCrdsaBackoffTimePassed = true;
    }

  NS_LOG_INFO ("SatRandomAccess::CrdsaHasBackoffTimePassed for allocation channel " << allocationChannel << ": " << hasCrdsaBackoffTimePassed);

  return hasCrdsaBackoffTimePassed;
}

void
SatRandomAccess::CrdsaReduceIdleBlocks (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  uint32_t idleBlocksLeft = m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaIdleBlocksLeft ();

  if (idleBlocksLeft > 0)
    {
      NS_LOG_INFO ("SatRandomAccess::CrdsaReduceIdleBlocks - Reducing allocation channel: " << allocationChannel << " idle blocks by one");
      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaIdleBlocksLeft (idleBlocksLeft - 1);
    }
}

void
SatRandomAccess::CrdsaReduceIdleBlocksForAllAllocationChannels ()
{
  NS_LOG_FUNCTION (this);

  for (uint32_t i = 0; i < m_numOfAllocationChannels; i++)
    {
      CrdsaReduceIdleBlocks (i);
    }
}

bool
SatRandomAccess::CrdsaIsAllocationChannelFree (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  if (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaIdleBlocksLeft () > 0)
    {
      NS_LOG_INFO ("SatRandomAccess::CrdsaIsAllocationChannelFree - Allocation channel: " << allocationChannel << " idle in effect");
      return false;
    }
  NS_LOG_INFO ("SatRandomAccess::CrdsaIsAllocationChannelFree - Allocation channel: " << allocationChannel << " free");
  return true;
}

bool
SatRandomAccess::CrdsaDoBackoff (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  bool doCrdsaBackoff = false;

  if (m_uniformRandomVariable->GetValue (0.0,1.0) < m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaBackoffProbability ())
    {
      doCrdsaBackoff = true;
    }

  NS_LOG_INFO ("SatRandomAccess::CrdsaDoBackoff for allocation channel " << allocationChannel << ": " << doCrdsaBackoff);

  return doCrdsaBackoff;
}

void
SatRandomAccess::CrdsaSetBackoffTimer (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaBackoffReleaseTime (Now ().GetSeconds ()
                                                                                                         + (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaBackoffTime () / 1000));

  CrdsaReduceIdleBlocks (allocationChannel);

  NS_LOG_INFO ("SatRandomAccess::CrdsaSetBackoffTimer - Setting backoff timer for allocation channel: " << allocationChannel);
}

SatRandomAccess::RandomAccessTxOpportunities_s
SatRandomAccess::CrdsaPrepareToTransmit (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  RandomAccessTxOpportunities_s txOpportunities;
  txOpportunities.txOpportunityType = SatRandomAccess::RA_DO_NOTHING;

  uint32_t maxUniquePackets = std::min (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaMaxUniquePayloadPerBlock (),
                                        m_numOfCandidatePacketsCb (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaPayloadBytes ()));

  /// TODO slots.first can be updated to take into account the reserved RA slots from MAC.
  /// This should be done by including the list of used slots in this SF as a parameter for the
  /// random access algorithm call. This functionality is needed with, e.g., multiple allocation channels
  std::pair <std::set<uint32_t>, std::set<uint32_t> > slots;
  uint32_t actualUniquePackets = 0;

  for (uint32_t i = 0; i < maxUniquePackets; i++)
    {
      if (CrdsaDoBackoff (allocationChannel))
        {
          CrdsaSetBackoffTimer (allocationChannel);
          break;
        }
      else
        {
          NS_LOG_INFO ("SatRandomAccess::CrdsaPrepareToTransmit - New Tx candidate for allocation channel: " << allocationChannel);

          if (CrdsaIsAllocationChannelFree (allocationChannel))
            {
              NS_LOG_INFO ("SatRandomAccess::CrdsaPrepareToTransmit - Preparing for transmission with allocation channel: " << allocationChannel);

              /// randomize instance slots for this unique packet
              slots = CrdsaRandomizeTxOpportunities (allocationChannel,slots);

              /// save the packet specific Tx opportunities into a vector
              txOpportunities.crdsaTxOpportunities.push_back (slots.second);

              /// increase the number of randomized unique packets
              actualUniquePackets++;

              if (m_areBuffersEmptyCb ())
                {
                  m_crdsaNewData = true;
                }
            }
        }
    }

  /// save the rest of the CRDSA Tx opportunity results
  txOpportunities.txOpportunityType = SatRandomAccess::RA_CRDSA_TX_OPPORTUNITY;

  CrdsaReduceIdleBlocks (allocationChannel);

  return txOpportunities;
}

void
SatRandomAccess::CrdsaIncreaseConsecutiveBlocksUsed (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaNumOfConsecutiveBlocksUsed (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaNumOfConsecutiveBlocksUsed () + 1);

  NS_LOG_INFO ("SatRandomAccess::CrdsaIncreaseConsecutiveBlocksUsed - Increasing the number of used consecutive blocks for allocation channel: " << allocationChannel);

  if (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaNumOfConsecutiveBlocksUsed () >= m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaMaxConsecutiveBlocksAccessed ())
    {
      NS_LOG_INFO ("SatRandomAccess::CrdsaIncreaseConsecutiveBlocksUsed - Maximum number of consecutive blocks reached, forcing idle blocks for allocation channel: " << allocationChannel);

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaIdleBlocksLeft (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaMinIdleBlocks ());

      m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaNumOfConsecutiveBlocksUsed (0);
    }
}

SatRandomAccess::RandomAccessTxOpportunities_s
SatRandomAccess::DoCrdsa (uint32_t allocationChannel)
{
  NS_LOG_FUNCTION (this);

  RandomAccessTxOpportunities_s txOpportunities;
  txOpportunities.txOpportunityType = SatRandomAccess::RA_DO_NOTHING;

  NS_LOG_INFO ("-------------------------------------");
  NS_LOG_INFO ("------ Running CRDSA algorithm ------");
  NS_LOG_INFO ("-------------------------------------");

  PrintVariables ();

  NS_LOG_INFO ("-------------------------------------");

  NS_LOG_INFO ("SatRandomAccess::DoCrdsa - Checking backoff period status...");

  if (CrdsaHasBackoffTimePassed (allocationChannel))
    {
      NS_LOG_INFO ("SatRandomAccess::DoCrdsa - Backoff period over, checking DAMA status...");

      if (!m_isDamaAvailableCb ())
        {
          NS_LOG_INFO ("SatRandomAccess::DoCrdsa - No DAMA, checking buffer status...");

          if (m_crdsaNewData)
            {
              m_crdsaNewData = false;

              NS_LOG_INFO ("SatRandomAccess::DoCrdsa - Evaluating back off...");

              if (CrdsaDoBackoff (allocationChannel))
                {
                  NS_LOG_INFO ("SatRandomAccess::DoCrdsa - Initial new data backoff triggered");
                  CrdsaSetBackoffTimer (allocationChannel);
                }
              else
                {
                  txOpportunities = CrdsaPrepareToTransmit (allocationChannel);
                }
            }
          else
            {
              txOpportunities = CrdsaPrepareToTransmit (allocationChannel);
            }

          if (txOpportunities.txOpportunityType == SatRandomAccess::RA_CRDSA_TX_OPPORTUNITY)
            {
              CrdsaIncreaseConsecutiveBlocksUsed (allocationChannel);
            }
          else if (txOpportunities.txOpportunityType == SatRandomAccess::RA_DO_NOTHING)
            {
              m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->SetCrdsaNumOfConsecutiveBlocksUsed (0);
            }
        }
      else
        {
          CrdsaReduceIdleBlocks (allocationChannel);
        }
    }
  else
    {
      CrdsaReduceIdleBlocks (allocationChannel);
    }

  NS_LOG_INFO ("--------------------------------------");
  NS_LOG_INFO ("------ CRDSA algorithm FINISHED ------");
  NS_LOG_INFO ("------ Result: " << txOpportunities.txOpportunityType << " ---------------------");
  NS_LOG_INFO ("--------------------------------------");

  return txOpportunities;
}

std::pair <std::set<uint32_t>, std::set<uint32_t> >
SatRandomAccess::CrdsaRandomizeTxOpportunities (uint32_t allocationChannel, std::pair <std::set<uint32_t>, std::set<uint32_t> > slots)
{
  NS_LOG_FUNCTION (this);

  std::pair<std::set<uint32_t>::iterator,bool> resultAllSlotsInFrame;
  std::pair<std::set<uint32_t>::iterator,bool> resultThisUniquePacket;

  std::set<uint32_t> emptySet;
  slots.second = emptySet;

  NS_LOG_INFO ("SatRandomAccess::CrdsaRandomizeTxOpportunities - Randomizing TX opportunities for allocation channel: " << allocationChannel);

  uint32_t successfulInserts = 0;
  uint32_t instances = m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaNumOfInstances ();
  while (successfulInserts < instances)
    {
      uint32_t slot = m_uniformRandomVariable->GetInteger (m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaMinRandomizationValue (),
                                                           m_randomAccessConf->GetAllocationChannelConfiguration (allocationChannel)->GetCrdsaMaxRandomizationValue ());

      resultAllSlotsInFrame = slots.first.insert (slot);

      if (resultAllSlotsInFrame.second)
        {
          successfulInserts++;

          resultThisUniquePacket = slots.second.insert (slot);

          if (resultAllSlotsInFrame.second)
            {
              NS_FATAL_ERROR ("SatRandomAccess::CrdsaRandomizeTxOpportunities - Slots out of sync, this should never happen");
            }
        }

      NS_LOG_INFO ("SatRandomAccess::CrdsaRandomizeTxOpportunities - Allocation channel: " << allocationChannel << " insert successful " << resultAllSlotsInFrame.second << " for TX opportunity slot: " << (*resultAllSlotsInFrame.first));
    }

  NS_LOG_INFO ("SatRandomAccess::CrdsaRandomizeTxOpportunities - Randomizing done");

  return slots;
}

/**
TODO The known DAMA capacity condition is different for control and data.
For control the known DAMA is limited to the SF about to start, i.e.,
the look ahead is one SF. For data the known DAMA allocation can be
one or more SF in the future, i.e., the look ahead contains all known
future DAMA allocations. With CRDSA the control packets have priority
over data packets.
*/

void
SatRandomAccess::SetIsDamaAvailableCallback (SatRandomAccess::IsDamaAvailableCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);

  m_isDamaAvailableCb = callback;
}

void
SatRandomAccess::SetAreBuffersEmptyCallback (SatRandomAccess::AreBuffersEmptyCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);

  m_areBuffersEmptyCb = callback;
}

void
SatRandomAccess::SetNumOfCandidatePacketsCallback (SatRandomAccess::NumOfCandidatePacketsCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);

  m_numOfCandidatePacketsCb = callback;
}

} // namespace ns3
