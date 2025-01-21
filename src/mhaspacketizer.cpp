/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2024 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

// System includes
#include <algorithm>
#include <iterator>
#include <set>

// External includes

// Internal includes
#include "mmtmhasparserlib/mhaspacketizer.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"
#include "mmtmhasparserlib/mhasframepacket.h"
#include "mmtmhasparserlib/mhastruncationpacket.h"
#include "logging.h"

using namespace mmt::mhasparserlib;

using CIndexedPackets = std::vector<std::pair<CUniqueMhasPacket, std::size_t>>;

static SMhasAccessUnit assembleAccessUnit(const CIndexedPackets& genericPackets,
                                          CIndexedPackets& streamPackets) {
  SMhasAccessUnit au{};
  auto genericIt = genericPackets.begin();
  auto streamIt = streamPackets.begin();

  while (genericIt != genericPackets.end() || streamIt != streamPackets.end()) {
    if (genericIt != genericPackets.end() &&
        (streamIt == streamPackets.end() || genericIt->second < streamIt->second)) {
      // copy generic packet
      ilo::ByteBuffer buffer{};
      genericIt->first->writePacket(buffer);
      CMhasParser parser{};
      parser.sync();
      parser.feed(buffer);
      parser.parsePackets();

      au.packets.push_back(parser.nextPacket());
      ++genericIt;
    }
    if (streamIt != streamPackets.end() &&
        (genericIt == genericPackets.end() || streamIt->second < genericIt->second)) {
      // move stream-specific packet
      au.packets.push_back(std::move(streamIt->first));
      ++streamIt;
    }
  }

  return au;
}

std::vector<SMhasAccessUnit> SMhasAccessUnit::splitStreams() && {
  CIndexedPackets genericPackets;
  std::map<uint64_t, CIndexedPackets> streamPackets;
  std::set<uint64_t> mpeghStreams;

  for (std::size_t i = 0; i < packets.size(); ++i) {
    auto& packet = packets[i];
    if (!packet) {
      continue;
    }
    if (packet->packetLabel() == 0U) {
      genericPackets.push_back(std::make_pair(std::move(packet), i));
    } else {
      if (static_cast<EMhasPacketType>(packet->packetType()) ==
          EMhasPacketType::PACTYP_MPEGH3DAFRAME) {
        mpeghStreams.emplace(packet->packetLabel());
      }
      streamPackets[packet->packetLabel()].push_back(std::make_pair(std::move(packet), i));
    }
  }

  std::deque<SMhasAccessUnit> streamUnits;
  for (auto packetLabel : mpeghStreams) {
    streamUnits.push_back(assembleAccessUnit(genericPackets, streamPackets.at(packetLabel)));
    auto& stream = streamUnits.back();
    stream.sampleRate = sampleRate;
    stream.duration = duration;
    stream.isIpf = isIpf;
  }

  std::vector<SMhasAccessUnit> result;
  result.assign(std::make_move_iterator(streamUnits.begin()),
                std::make_move_iterator(streamUnits.end()));
  return result;
}

struct CMhasPacketizer::SStreamConfig {
  uint32_t sampleRate;
  uint32_t defaultFrameSize;

  bool operator==(const SStreamConfig& other) const noexcept {
    return sampleRate == other.sampleRate && defaultFrameSize == other.defaultFrameSize;
  }
};

CMhasPacketizer::CMhasPacketizer(const SConfig& config) : m_config(config) {}
CMhasPacketizer::~CMhasPacketizer() noexcept = default;

void CMhasPacketizer::feed(const ilo::ByteBuffer& vector) {
  m_parser.feed(vector);
}

void CMhasPacketizer::feed(const uint8_t* rawBuffer, size_t size) {
  m_parser.feed(rawBuffer, size);
}

uint32_t CMhasPacketizer::numAccessUnitsAvailable() const {
  return static_cast<uint32_t>(m_parsedAus.size());
}

uint32_t CMhasPacketizer::numBytesPending() const {
  return m_parser.numBytesPending();
}

bool CMhasPacketizer::isSynced() const {
  return m_parser.isSynced();
}

void CMhasPacketizer::sync() {
  return m_parser.sync();
}

void CMhasPacketizer::reset() {
  m_parser.reset();
  m_configs.clear();
}

//! Returns the iterator after the last packet belonging to the access unit and whether to process
//! (second value true) or discard (second value false) the packets.
static std::pair<CPacketDeque::iterator, bool> findEndOfAccessUnit(
    CPacketDeque& pendingPackets,
    const std::map<uint64_t, CMhasPacketizer::SStreamConfig>& streamConfigs, bool strictMode) {
  std::set<uint64_t> newPacketLabels;
  std::set<uint64_t> presentPacketLabels;
  auto it = pendingPackets.begin();
  for (; it != pendingPackets.end(); ++it) {
    if (!*it) {
      continue;
    }
    auto packetLabel = (*it)->packetLabel();

    switch (static_cast<EMhasPacketType>((*it)->packetType())) {
      case EMhasPacketType::PACTYP_SYNC:
        if (it != pendingPackets.begin()) {
          // The SYNC packet starts a new access unit, so process all preceding packets
          return std::make_pair(it, true /* process */);
        }
        break;
      case EMhasPacketType::PACTYP_MPEGH3DACFG:
        // There is a config for this label available, so frame packets are allowed for it
        if (newPacketLabels.find(packetLabel) != newPacketLabels.end()) {
          ILO_ASSERT(
              !strictMode,
              "Multiple MHAS config packets found in a single access unit for label: %" PRIu64,
              packetLabel);
          ILO_LOG_WARNING(
              "MHAS config packet overwrites previous config packet in the same access unit for "
              "label: %" PRIu64,
              packetLabel);
        }
        newPacketLabels.emplace(packetLabel);
        break;
      case EMhasPacketType::PACTYP_MPEGH3DAFRAME:
        // If there are new config packets for this access unit, only accept frame packets for
        // those, otherwise accept for previous config packets
        if (!newPacketLabels.empty()) {
          if (newPacketLabels.find(packetLabel) == newPacketLabels.end()) {
            ILO_ASSERT(!strictMode,
                       "No MHAS config packet found before MHAS frame packet for label: %" PRIu64,
                       packetLabel);
            ILO_LOG_WARNING(
                "Discarding MHAS frame packet processed before MHAS config packet for label: "
                "%" PRIu64,
                packetLabel);
            return std::make_pair(it + 1, false /* drop */);
          }
        } else if (streamConfigs.find(packetLabel) == streamConfigs.end()) {
          ILO_ASSERT(!strictMode,
                     "No MHAS config packet found before MHAS frame packet for label: %" PRIu64,
                     packetLabel);
          ILO_LOG_WARNING(
              "Discarding MHAS frame packet processed before MHAS config packet for label: "
              "%" PRIu64,
              packetLabel);
          return std::make_pair(it + 1, false /* drop */);
        }
        if (presentPacketLabels.find(packetLabel) != presentPacketLabels.end()) {
          // Second frame packet for a packet label already processed, so end access unit before
          // this frame packet
          ILO_ASSERT(!strictMode,
                     "No MHAS frame packets for all packet labels processed before further MHAS "
                     "frame packet for packet label: %" PRIu64,
                     packetLabel);
          ILO_LOG_WARNING(
              "Did not process MHAS frame packets for all packet labels in an access unit before "
              "starting new access unit with MHAS frame packet for packet label: %" PRIu64,
              packetLabel);
          return std::make_pair(it, true /* process */);
        }
        presentPacketLabels.emplace(packetLabel);

        if (!newPacketLabels.empty() && presentPacketLabels.size() == newPacketLabels.size()) {
          // There is a frame for all new configs, thus the access unit is finished
          return std::make_pair(it + 1, true /* process */);
        } else if (newPacketLabels.empty() && presentPacketLabels.size() == streamConfigs.size()) {
          // There is a frame for all previous configs, thus the access unit is finished
          return std::make_pair(it + 1, true /* process */);
        }
        break;
      default:
        // Don't care about any other packet types here
        break;
    }
  }
  return std::make_pair(pendingPackets.begin(), false /* no access unit */);
}

static CMhasPacketizer::SStreamConfig extractConfig(const CMhasConfigPacket& configPacket) {
  CMhasConfigPacket::SConfig configPacketInfo = configPacket.mhasConfigInfo();
  ILO_ASSERT(configPacketInfo.profileLevelIndication >= 0x0B &&
                 configPacketInfo.profileLevelIndication <= 0x14,
             "Only Low Complexity and Baseline bitstreams are supported.");
  ILO_ASSERT(configPacketInfo.outputSamplingFrequency != -1,
             "Unable to extract output sample rate.");
  ILO_ASSERT(configPacketInfo.outputFramesize != -1, "Unable to extract frame size.");

  return {static_cast<uint32_t>(configPacketInfo.outputSamplingFrequency),
          static_cast<uint32_t>(configPacketInfo.outputFramesize)};
}

static void updateAndExtractConfigs(SMhasAccessUnit& au,
                                    std::map<uint64_t, CMhasPacketizer::SStreamConfig>& configs) {
  std::map<uint64_t, CMhasPacketizer::SStreamConfig> newConfigs;
  std::map<uint64_t, uint32_t> truncations;

  bool isAnyIpf = false;
  bool isAllIpf = true;

  for (const auto& packet : au.packets) {
    if (!packet) {
      continue;
    }

    switch (static_cast<EMhasPacketType>(packet->packetType())) {
      case EMhasPacketType::PACTYP_MPEGH3DACFG: {
        const auto* configPacket = dynamic_cast<const CMhasConfigPacket*>(packet.get());
        ILO_ASSERT(configPacket, "MHAS PACTYP_MPEGH3DACFG packet is not a config packet");
        newConfigs[packet->packetLabel()] = extractConfig(*configPacket);
        break;
      }
      case EMhasPacketType::PACTYP_MPEGH3DAFRAME: {
        const auto* framePacket = dynamic_cast<const CMhasFramePacket*>(packet.get());
        ILO_ASSERT(framePacket, "MHAS PACTYP_MPEGH3DAFRAME packet is not a frame packet");
        bool isIpf = framePacket->isIPF();
        isAnyIpf = isAnyIpf || isIpf;
        isAllIpf = isAllIpf && isIpf;
        break;
      }
      case EMhasPacketType::PACTYP_AUDIOTRUNCATION: {
        const auto* truncPacket = dynamic_cast<const CMhasTruncationPacket*>(packet.get());
        ILO_ASSERT(truncPacket, "MHAS PACTYP_AUDIOTRUNCATION packet is not a truncation packet");
        auto label = packet->packetLabel();
        if (truncPacket->isActive() &&
            (newConfigs.find(label) != newConfigs.end() ||
             (newConfigs.empty() && configs.find(label) != configs.end()))) {
          // Only care about and apply "active" truncations for packet labels containing MPEG-H 3DA
          // streams (not e.g. raw PCM data).
          truncations[label] += truncPacket->truncatedSamples();
        }
        break;
      }
      default:
        // Don't care about any other packet types here
        break;
    }
  }

  ILO_ASSERT(isAllIpf == isAnyIpf,
             "Multi-stream MHAS with mixed IPF and non-IPF frame packets is not supported");
  if (!newConfigs.empty()) {
    configs = std::move(newConfigs);
  }

  auto& firstConfig = configs.begin()->second;
  ILO_ASSERT(
      std::all_of(configs.begin(), configs.end(),
                  [firstConfig](const std::pair<uint64_t, CMhasPacketizer::SStreamConfig>& config) {
                    return config.second == firstConfig;
                  }),
      "Multi-stream MHAS with mixed sample timing configurations is not supported");

  if (!truncations.empty()) {
    uint32_t firstTruncation = truncations.begin()->second;
    ILO_ASSERT(std::all_of(truncations.begin(), truncations.end(),
                           [firstTruncation](const std::pair<uint64_t, uint32_t>& pair) {
                             return pair.second == firstTruncation;
                           }),
               "Multi-stream MHAS with mixed truncation durations is not supported");
  }

  au.sampleRate = firstConfig.sampleRate;
  au.duration =
      firstConfig.defaultFrameSize - (truncations.empty() ? 0U : truncations.begin()->second);
  au.isIpf = isAllIpf;
}

void CMhasPacketizer::parseAccessUnits() {
  m_parser.parsePackets();

  {
    auto newPackets = m_parser.allAvailablePackets();
    m_pendingPackets.insert(m_pendingPackets.end(), std::make_move_iterator(newPackets.begin()),
                            std::make_move_iterator(newPackets.end()));
  }

  do {
    auto unitEnd = findEndOfAccessUnit(m_pendingPackets, m_configs, m_config.strictMode);
    if (unitEnd.first == m_pendingPackets.begin()) {
      // no access unit found
      return;
    } else if (!unitEnd.second) {
      // drop all the packets up to the iterator instead of processing them
      m_pendingPackets.erase(m_pendingPackets.begin(), unitEnd.first);
      continue;
    }

    SMhasAccessUnit au{};
    au.packets.assign(std::make_move_iterator(m_pendingPackets.begin()),
                      std::make_move_iterator(unitEnd.first));
    m_pendingPackets.erase(m_pendingPackets.begin(), unitEnd.first);
    updateAndExtractConfigs(au, m_configs);
    m_parsedAus.push_back(std::move(au));
  } while (!m_pendingPackets.empty());
}

SMhasAccessUnit CMhasPacketizer::nextAccessUnit() {
  if (!m_parsedAus.empty()) {
    auto packet = std::move(m_parsedAus.front());
    m_parsedAus.pop_front();
    return packet;
  }
  return {};
}

std::deque<SMhasAccessUnit> CMhasPacketizer::allAvailableAccessUnits() {
  std::deque<SMhasAccessUnit> deque;
  deque.swap(m_parsedAus);
  return deque;
}
