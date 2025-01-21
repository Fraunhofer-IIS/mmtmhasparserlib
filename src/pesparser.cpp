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
#include <array>
#include <cstring>

// External includes
#include "ilo/bitparser.h"
#include "ilo/memory.h"

// Internal includes
#include "mmtmhasparserlib/pesparser.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"
#include "mmtmhasparserlib/mhasframepacket.h"
#include "mmtmhasparserlib/mhastruncationpacket.h"
#include "logging.h"

using namespace mmt::mhasparserlib;

static const std::array<uint8_t, 3> START_CODE_PREFIX = {0x00, 0x00, 0x01};

struct CPesParser::SPesPacket {
  SPesPacket() = default;

  SPesPacket(uint64_t ptsValue, uint64_t dtsValue, ilo::ByteBuffer&& buffer)
      : streamId(0 /* don't care */),
        pts(ptsValue),
        dts(dtsValue),
        escr(0 /* unknown */),
        esRate(0 /* unknown */),
        payload(std::move(buffer)) {}

  uint8_t streamId;
  uint64_t pts;
  uint64_t dts;
  uint64_t escr;
  uint32_t esRate;

  ilo::ByteBuffer payload;
};

struct CPesParser::SMhasConfig {
  uint64_t expectedAbsolutePts;
  uint64_t expectedPtsValue;
  uint64_t expectedAbsoluteDts;
  uint64_t expectedDtsValue;
  uint32_t lastBitrate;

  uint32_t sampleRate;
  uint32_t defaultFrameSize;

  uint32_t nextFrameTruncation;
};

CPesParser::CPesParser() = default;
CPesParser::~CPesParser() noexcept = default;

void CPesParser::feed(const ilo::ByteBuffer& vector) {
  m_buffer.insert(m_buffer.end(), vector.begin(), vector.end());
  parseFullPesPackets();
}

void CPesParser::feed(const uint8_t* rawBuffer, size_t size) {
  m_buffer.insert(m_buffer.end(), rawBuffer, rawBuffer + size);
  parseFullPesPackets();
}

void CPesParser::feedPesPacket(uint64_t pts, uint64_t dts, const uint8_t* pesPayloadBuffer,
                               size_t pesPayloadSize) {
  m_isSynced = true;
  m_pesPackets.push_back(ilo::make_unique<SPesPacket>(
      pts, dts, ilo::ByteBuffer(pesPayloadBuffer, pesPayloadBuffer + pesPayloadSize)));
}

std::size_t CPesParser::numAccessUnitsAvailable() const {
  return m_parsedAus.size();
}

std::size_t CPesParser::numBytesPending() const {
  return m_buffer.size();
}

std::size_t CPesParser::numPesPacketsPending() const {
  return m_pesPackets.size();
}

bool CPesParser::isSynced() const {
  return m_isSynced && m_mhasParser.isSynced();
}

void CPesParser::reset() {
  m_buffer.clear();
  m_pesPackets.clear();
  m_pendingAu = {};
  m_mhasParser.reset();
  m_parsedAus.clear();
  m_streamConfig = {};
  m_isSynced = false;
}

static void warnStreamError(bool condition, const char* message) {
  if (!condition) {
    ILO_LOG_WARNING(message);
  }
}

static uint64_t parseTimestamp(ilo::CBitParser& bitParser) {
  uint64_t timestamp = bitParser.read<uint64_t>(3) << 30;
  warnStreamError(bitParser.read<uint8_t>(1) == 1, "Invalid value for marker bit in PES timestamp");
  timestamp |= bitParser.read<uint64_t>(15) << 15;
  warnStreamError(bitParser.read<uint8_t>(1) == 1, "Invalid value for marker bit in PES timestamp");
  timestamp |= bitParser.read<uint64_t>(15);
  warnStreamError(bitParser.read<uint8_t>(1) == 1, "Invalid value for marker bit in PES timestamp");
  return timestamp;
}

static std::unique_ptr<CPesParser::SPesPacket> parseNextPesPacket(
    ilo::ByteBuffer::const_iterator& it, ilo::ByteBuffer::const_iterator end) {
  if (std::distance(it, end) < 6U /* minimal PES header size */) {
    return nullptr;
  }

  ILO_ASSERT(std::equal(it, it + START_CODE_PREFIX.size(), START_CODE_PREFIX.begin()),
             "PES packet does not start with PES start code prefix");
  it += START_CODE_PREFIX.size();

  ilo::CBitParser bitParser{it, end};
  auto packet = ilo::make_unique<CPesParser::SPesPacket>();
  packet->streamId = bitParser.read<uint8_t>(8);
  ++it;

  auto packetLength = bitParser.read<uint16_t>(16);
  it += 2;
  ILO_ASSERT(packetLength > 0,
             "Unbound PES packets are only allowed for video, not for MPEG-H audio payload");
  if (std::distance(it, end) < packetLength) {
    // packet does not fit, reset the iterator
    it -= 6;
    return nullptr;
  }

  auto packetEnd = it + packetLength;
  ILO_ASSERT(bitParser.read<uint8_t>(2) == 0x02,
             "Invalid PES packet type for MPEG-H audio payload");
  ILO_ASSERT(bitParser.read<uint8_t>(2) == 0x00, "Scrambled PES packet payload is not supported");
  bitParser.seek(4, ilo::EPosType::cur);  // skip some uninteresting flags
  ++it;

  auto timestampFlags = bitParser.read<uint8_t>(2);
  bool escrFlag = bitParser.read<uint8_t>(1) == 1;
  bool esRateFlag = bitParser.read<uint8_t>(1) == 1;
  bitParser.seek(4, ilo::EPosType::cur);  // skip some uninteresting flags
  ++it;

  auto headerLength = bitParser.read<uint8_t>(8);
  ++it;
  auto payloadIt = it + headerLength;

  if (timestampFlags == 0x02) {
    warnStreamError(bitParser.read<uint8_t>(4) == 0x02,
                    "Invalid bit pattern for PES PTS header field");
    packet->pts = parseTimestamp(bitParser);
    packet->dts = packet->pts;
    it += 5;
  } else if (timestampFlags == 0x03) {
    warnStreamError(bitParser.read<uint8_t>(4) == 0x03,
                    "Invalid bit pattern for PES PTS header field");
    packet->pts = parseTimestamp(bitParser);
    warnStreamError(bitParser.read<uint8_t>(4) == 0x01,
                    "Invalid bit pattern for PES DTS header field");
    packet->dts = parseTimestamp(bitParser);
    it += 10;
  }
  if (escrFlag) {
    warnStreamError(bitParser.read<uint8_t>(2) == 0x03,
                    "Invalid bit pattern for PES ESCR header field");
    uint64_t escrBase = parseTimestamp(bitParser);
    uint64_t escrExtension = bitParser.read<uint64_t>(9);
    ILO_ASSERT(bitParser.read<uint8_t>(1) == 1,
               "Invalid value for marker bit in PES ESCR header field");
    packet->escr = escrBase * 300 + escrExtension;
    it += 6;
  }
  if (esRateFlag) {
    warnStreamError(bitParser.read<uint8_t>(1) == 1, "Invalid value for marker bit in ES rate");
    packet->esRate = bitParser.read<uint32_t>(22);
    warnStreamError(bitParser.read<uint8_t>(1) == 1, "Invalid value for marker bit in ES rate");
    it += 3;
  }

  packet->payload.assign(payloadIt, packetEnd);
  it = packetEnd;

  return packet;
}

static void updateStreamConfig(CPesParser::SMhasConfig& config,
                               const CMhasConfigPacket& configPacket) {
  CMhasConfigPacket::SConfig configPacketInfo = configPacket.mhasConfigInfo();
  ILO_ASSERT(configPacketInfo.profileLevelIndication >= 0x0B &&
                 configPacketInfo.profileLevelIndication <= 0x14,
             "Only Low Complexity and Baseline bitstreams are supported.");
  ILO_ASSERT(configPacketInfo.outputSamplingFrequency != -1,
             "Unable to extract output sample rate.");
  ILO_ASSERT(configPacketInfo.outputFramesize != -1, "Unable to extract frame size.");

  auto newSampleRate = static_cast<uint32_t>(configPacketInfo.outputSamplingFrequency);
  ILO_ASSERT(config.sampleRate == 0 || config.sampleRate == newSampleRate,
             "Output sampling rate must stay constant within a file.");
  config.sampleRate = newSampleRate;
  config.defaultFrameSize = static_cast<uint32_t>(configPacketInfo.outputFramesize);
}

static uint64_t updateTimestamps(uint64_t& expectedAbsolute, uint64_t& expectedValue,
                                 uint64_t value, uint32_t duration) {
  if (expectedValue > value) {
    // wrap-around
    auto difference = (uint64_t{1U} << 33U) + value - expectedValue;
    expectedAbsolute += difference;
  } else if (expectedValue < value) {
    // gap
    auto difference = value - expectedValue;
    expectedAbsolute += difference;
  }
  auto result = expectedAbsolute;
  expectedAbsolute += duration;
  expectedValue = value + duration;
  return result;
}

static void updateAccessUnitInfo(CPesParser::SMhasConfig& config, SPesMhasAccessUnit& au,
                                 CPesParser::SPesPacket& packet, bool auStartsInPreviousPes) {
  au.escr = packet.escr;
  if (packet.esRate) {
    au.bitrate = config.lastBitrate = packet.esRate * 50 /* 50 B/s steps */ * 8 /* 8 bit */;
  } else {
    au.bitrate = config.lastBitrate;
  }

  au.sampleRate = config.sampleRate;
  au.duration = config.defaultFrameSize < config.nextFrameTruncation
                    ? 0U
                    : config.defaultFrameSize - config.nextFrameTruncation;
  config.nextFrameTruncation = 0;

  auto durationInTicks = au.duration * SPesMhasAccessUnit::TIMESCALE / au.sampleRate;
  if (auStartsInPreviousPes) {
    // The access unit starts in the last PES packet and thus uses its next expected timestamp
    au.pts = updateTimestamps(config.expectedAbsolutePts, config.expectedPtsValue, au.pts,
                              durationInTicks);
    au.dts = updateTimestamps(config.expectedAbsoluteDts, config.expectedDtsValue, au.dts,
                              durationInTicks);
  } else {
    au.pts = updateTimestamps(config.expectedAbsolutePts, config.expectedPtsValue, packet.pts,
                              durationInTicks);
    au.dts = updateTimestamps(config.expectedAbsoluteDts, config.expectedDtsValue, packet.dts,
                              durationInTicks);

    // update the packet timestamps for multi-AU case
    packet.pts += durationInTicks;
    packet.dts += durationInTicks;
  }
}

void CPesParser::parseAccessUnits() {
  for (auto& pesPacket : m_pesPackets) {
    // An access unit starts in the PES packet in which its first byte is located (i.e. the first
    // byte of the first MHAS packet in the access unit, not the first byte of the MHAS frame
    // packet). So if any bytes of a MHAS packet starting in the previous PES packet are still
    // pending, the MHAS access unit starts in the previous PES packet and thus uses its timestamps.
    bool auStartsInPreviousPes = m_mhasParser.numBytesPending() || m_pendingAu;

    if (!auStartsInPreviousPes) {
      // Store PES packet timestamps of first MHAS packets for access unit
      m_pendingAu.pts = pesPacket->pts;
      m_pendingAu.dts = pesPacket->dts;
    }

    m_mhasParser.feed(pesPacket->payload);
    m_mhasParser.parsePackets();

    while (auto mhasPacket = m_mhasParser.nextPacket()) {
      if (const auto* framePacket = dynamic_cast<const CMhasFramePacket*>(mhasPacket.get())) {
        ILO_ASSERT(m_streamConfig,
                   "No initial MHAS config packet found before the first MHAS frame packet.");
        m_pendingAu.isIpf = framePacket->isIPF();
        updateAccessUnitInfo(*m_streamConfig, m_pendingAu, *pesPacket, auStartsInPreviousPes);
        m_pendingAu.packets.push_back(std::move(mhasPacket));
        m_parsedAus.push_back(std::move(m_pendingAu));
        // Start new container for next MHAS access unit
        m_pendingAu = {};
        // Store next expected timestamps in case the MHAS access unit starts in the same PES packet
        m_pendingAu.pts = m_streamConfig->expectedPtsValue;
        m_pendingAu.dts = m_streamConfig->expectedPtsValue;
        auStartsInPreviousPes = false;
        continue;
      } else if (const auto* configPacket =
                     dynamic_cast<const CMhasConfigPacket*>(mhasPacket.get())) {
        if (!m_streamConfig) {
          m_streamConfig = ilo::make_unique<SMhasConfig>();
        }
        updateStreamConfig(*m_streamConfig, *configPacket);
      } else if (const auto* truncPacket =
                     dynamic_cast<const CMhasTruncationPacket*>(mhasPacket.get())) {
        if (truncPacket->isActive()) {
          ILO_ASSERT(
              m_streamConfig,
              "No initial MHAS config packet found before the first MHAS truncation packet.");
          m_streamConfig->nextFrameTruncation += truncPacket->truncatedSamples();
        }
      }
      m_pendingAu.packets.push_back(std::move(mhasPacket));
    }
  }
  m_pesPackets.clear();
}

SPesMhasAccessUnit CPesParser::nextAccessUnit() {
  if (!m_parsedAus.empty()) {
    auto au = std::move(m_parsedAus.front());
    m_parsedAus.pop_front();
    return au;
  }
  return {};
}

std::deque<SPesMhasAccessUnit> CPesParser::allAvailableAccessUnits() {
  std::deque<SPesMhasAccessUnit> deque;
  deque.swap(m_parsedAus);
  return deque;
}

ilo::ByteBuffer::const_iterator CPesParser::syncIfNecessary(ilo::ByteBuffer::const_iterator begin,
                                                            ilo::ByteBuffer::const_iterator end) {
  if (!m_isSynced) {
    auto it = std::search(begin, end, START_CODE_PREFIX.begin(), START_CODE_PREFIX.end());
    if (it != end) {
      m_isSynced = true;
    }
    return it;
  }
  return begin;
}

void CPesParser::parseFullPesPackets() {
  auto it = syncIfNecessary(m_buffer.begin(), m_buffer.end());
  m_buffer.erase(m_buffer.begin(), it);
  if (!m_isSynced || m_buffer.empty()) {
    return;
  }

  it = m_buffer.begin();
  while (auto pesPacket = parseNextPesPacket(it, m_buffer.end())) {
    m_pesPackets.push_back(std::move(pesPacket));
  }
  m_buffer.erase(m_buffer.begin(), it);
}
