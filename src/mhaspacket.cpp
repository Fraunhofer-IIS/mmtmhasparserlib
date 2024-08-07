/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2017 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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

// system includes
#if defined(_MSC_VER) && _MSC_VER < 1920
// Silence VS warning for passing (completely valid) pointers to std::copy
#define _SCL_SECURE_NO_WARNINGS
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ilo includes
#include "ilo/bitbuffer.h"
#include "ilo/bitparser.h"
#include "ilo/memory.h"

// project specific includes
#include "logging.h"
#include "mmtmhasparserlib/mhaspacket.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"
#include "mmtmhasparserlib/mhascrc16packet.h"
#include "mmtmhasparserlib/mhastruncationpacket.h"
#include "mmtmhasparserlib/mhasframepacket.h"
#include "mmtmhasparserlib/mhassyncpacket.h"
#include "mmtmhasparserlib/mhasasipacket.h"
#include "mmtmhasparserlib/mhasmarkerpacket.h"
#include "mmtmhasparserlib/mhasutilities.h"

using namespace mmt::mhasparserlib;

const std::map<EMhasPacketType, uint32_t> mmt::mhasparserlib::IPF_PACKETS_ORDER{
    {EMhasPacketType::PACTYP_SYNC, 0},
    {EMhasPacketType::PACTYP_MARKER, 8},
    {EMhasPacketType::PACTYP_MPEGH3DACFG, 1},
    {EMhasPacketType::PACTYP_AUDIOSCENEINFO, 2},
    {EMhasPacketType::PACTYP_AUDIOTRUNCATION, 4},
    {EMhasPacketType::PACTYP_MPEGH3DAFRAME, 5}};

std::string mmt::mhasparserlib::packetTypeToString(EMhasPacketType packetType) {
  switch (packetType) {
    case EMhasPacketType::PACTYP_FILLDATA:
      return "FILLDATA";
    case EMhasPacketType::PACTYP_MPEGH3DACFG:
      return "MPEGH3DACFG";
    case EMhasPacketType::PACTYP_MPEGH3DAFRAME:
      return "MPEGH3DAFRAME";
    case EMhasPacketType::PACTYP_AUDIOSCENEINFO:
      return "AUDIOSCENEINFO";
    case EMhasPacketType::PACTYP_SYNC:
      return "SYNC";
    case EMhasPacketType::PACTYP_CRC16:
      return "CRC16";
    case EMhasPacketType::PACTYP_AUDIOTRUNCATION:
      return "AUDIOTRUNCATION";
    case EMhasPacketType::PACTYP_MARKER:
      return "MARKER";
    case EMhasPacketType::PACTYP_FRAMELENGTH:
      return "FRAMELENGTH";
    default:
      return "UNKNOWN";
  }
}

class CCRC16 {
 public:
  CCRC16(uint16_t crcPolynom, uint16_t crcStartValue);

  uint16_t calculateCRC(const ilo::ByteBuffer& buffer);

 private:
  std::array<uint16_t, 256> m_lookupTable;
  uint16_t m_crcStartValue;
};

CCRC16::CCRC16(uint16_t crcPolynom, uint16_t crcStartValue) : m_crcStartValue(crcStartValue) {
  m_lookupTable.fill(0);

  for (uint16_t i = 0; i < m_lookupTable.size(); ++i) {
    uint16_t value = static_cast<uint16_t>(i << 8u);
    for (int8_t j = 7; j >= 0; j--) {
      if ((value & 0x8000u) > 0) {
        value = static_cast<uint16_t>((value << 1u) ^ crcPolynom);
      } else {
        value = static_cast<uint16_t>(value << 1u);
      }
    }

    m_lookupTable[i] = value;
  }
}

uint16_t CCRC16::calculateCRC(const ilo::ByteBuffer& buffer) {
  uint16_t crc = m_crcStartValue;

  for (auto byte : buffer) {
    crc = static_cast<uint16_t>((crc << 8u) ^
                                m_lookupTable[static_cast<std::size_t>((crc >> 8u) ^ byte)]);
  }
  return crc;
}

static uint32_t escapedValueWriteSize(uint64_t value, uint32_t first, uint32_t second,
                                      uint32_t third) {
  uint32_t size = first;
  if (value >= (uint64_t{2u} << (first - 1u)) - 1u) {
    size += second;
    value -= (uint64_t{2u} << (first - 1u)) - 1u;

    if (value >= (uint64_t{2u} << (second - 1u)) - 1u) {
      size += third;
    }
  }
  return size;
}

CMhasPacket::CMhasPacket(ilo::ByteBuffer::const_iterator& begin,
                         ilo::ByteBuffer::const_iterator end) {
  ILO_ASSERT_WITH(begin < end, std::invalid_argument, "Invalid iterators provided (begin >= end).");

  ilo::CBitParser bitBuffer(begin, end);
  m_packetType = static_cast<uint32_t>(readEscapedValue(bitBuffer, 3, 8, 8));
  m_packetLabel = readEscapedValue(bitBuffer, 2, 8, 32);
  uint64_t packetLength = readEscapedValue(bitBuffer, 11, 24, 24);

  uint32_t read = bitBuffer.nofReadBits() / 8u;
  ILO_ASSERT(static_cast<std::size_t>(end - begin) >= read + packetLength,
             "Payload is not completely covered by begin and end.");

  begin += read;
  m_payload = ilo::ByteBuffer(begin, begin + static_cast<std::ptrdiff_t>(packetLength));
  begin += static_cast<std::ptrdiff_t>(packetLength);
}

CMhasPacket::CMhasPacket(uint32_t packetType) : m_packetLabel(1u), m_packetType(packetType) {}

CUniqueMhasPacket CMhasPacket::s_parseNextPacket(ilo::ByteBuffer::const_iterator& begin,
                                                 ilo::ByteBuffer::const_iterator end,
                                                 const bool audioPreRollPresent) {
  if (begin == end) {
    return nullptr;
  }

  ILO_ASSERT_WITH(begin < end, std::invalid_argument, "Invalid iterators provided (begin >= end)");
  ilo::CBitParser bitBuffer(begin, end);

  EMhasPacketType packetType = {};
  uint64_t packetLength = 0;

  try {
    packetType = static_cast<EMhasPacketType>(readEscapedValue(bitBuffer, 3, 8, 8));

    // Parse packet label -> will later be reparsed
    readEscapedValue(bitBuffer, 2, 8, 32);
    packetLength = readEscapedValue(bitBuffer, 11, 24, 24);
  } catch (const std::exception& /*e*/) {
    return nullptr;
  }

  uint32_t headerLength = bitBuffer.nofReadBits() / 8u;

  std::size_t packetSize = headerLength + static_cast<std::size_t>(packetLength);
  if (static_cast<std::size_t>(end - begin) < packetSize) {
    return nullptr;
  }

  switch (packetType) {
    case EMhasPacketType::PACTYP_CRC16:
      return ilo::make_unique<CMhasCRC16Packet>(begin, end);
    case EMhasPacketType::PACTYP_AUDIOTRUNCATION:
      return ilo::make_unique<CMhasTruncationPacket>(begin, end);
    case EMhasPacketType::PACTYP_MPEGH3DAFRAME:
      return ilo::make_unique<CMhasFramePacket>(begin, end, audioPreRollPresent);
    case EMhasPacketType::PACTYP_AUDIOSCENEINFO:
      return ilo::make_unique<CMhasAsiPacket>(begin, end);
    case EMhasPacketType::PACTYP_MPEGH3DACFG:
      return ilo::make_unique<CMhasConfigPacket>(begin, end);
    case EMhasPacketType::PACTYP_SYNC:
      return ilo::make_unique<CMhasSyncPacket>(begin, end);
    case EMhasPacketType::PACTYP_MARKER:
      return ilo::make_unique<CMhasMarkerPacket>(begin, end);
    default:
      return ilo::make_unique<CMhasPacket>(begin, end);
  }
}

void CMhasPacket::payload(ilo::ByteBuffer::const_iterator begin,
                          ilo::ByteBuffer::const_iterator end) {
  ILO_ASSERT_WITH(begin <= end, std::invalid_argument, "Invalid iterators provided (end < begin).");
  m_payload = ilo::ByteBuffer(begin, end);
}

void CMhasPacket::packetLabel(const uint64_t label) {
  m_packetLabel = label;
}

std::string CMhasPacket::toString(bool dumpPayload) {
  std::stringstream stream;

  stream << packetTypeToString(EMhasPacketType(m_packetType)) << ", Packet-Name: " << packetName()
         << ", Packet-Label: " << m_packetLabel << ", Payload-Length: " << m_payload.size()
         << ", Header-Length: " << calculatePacketSize() - m_payload.size();

  if (dumpPayload) {
    stream << ", Payload:";

    for (auto byte : m_payload) {
      stream << " 0x" << std::hex << static_cast<uint16_t>(byte) << std::dec;
    }
  }

  std::string specificInfo = packetSpecificInfo();
  if (!specificInfo.empty()) {
    stream << "\n - Packet specific info: " << specificInfo;
  }

  return stream.str();
}

void CMhasPacket::writePacket(ilo::ByteBuffer& vector) const {
  std::size_t newSize = calculatePacketSize();

  ILO_ASSERT(newSize != 0, "Packet doesn't contain payload or payload is too big.");

  vector.resize(newSize);
  writePacket(vector.data(), vector.size());
}

void CMhasPacket::writePacket(ilo::ByteBuffer::iterator begin,
                              ilo::ByteBuffer::iterator end) const {
  ILO_ASSERT_WITH(end >= begin, std::invalid_argument, "Begin iterator > end iterator.");

  std::size_t packetSize = calculatePacketSize();
  ILO_ASSERT_WITH(static_cast<std::size_t>(end - begin) == packetSize, std::invalid_argument,
                  "Provided iterator range is too small.");

  auto bytesWritten = writePacket(&(*begin), static_cast<std::size_t>(end - begin));

  ILO_ASSERT(bytesWritten == packetSize, "Packet was not completely written.");
}

std::size_t CMhasPacket::writePacket(uint8_t* rawBuffer, std::size_t rawBufferSize) const {
  std::size_t bytes = calculatePacketSize();

  ILO_ASSERT_WITH(bytes <= rawBufferSize, std::invalid_argument, "Provided buffer is too small.");
  ilo::CBitBuffer bitBuffer(rawBuffer, static_cast<uint32_t>(rawBufferSize));

  writeEscapedValue(bitBuffer, m_packetType, 3, 8, 8);
  writeEscapedValue(bitBuffer, m_packetLabel, 2, 8, 32);
  writeEscapedValue(bitBuffer, m_payload.size(), 11, 24, 24);

  ILO_ASSERT(bitBuffer.tell() % 8u == 0u, "Wrote invalid amount of bits.");

  auto* payloadStart = rawBuffer + bitBuffer.tell() / 8;
  ILO_ASSERT(static_cast<std::size_t>(payloadStart - rawBuffer) + m_payload.size() == bytes,
             "Size calculation is wrong.");

  std::copy_n(m_payload.begin(), m_payload.size(), payloadStart);
  return bytes;
}

uint32_t CMhasPacket::calculatePacketSize() const {
  uint64_t bits = 0u;
  bits += escapedValueWriteSize(m_packetType, 3, 8, 8);
  bits += escapedValueWriteSize(m_packetLabel, 2, 8, 32);
  bits += escapedValueWriteSize(m_payload.size(), 11, 24, 24);

  ILO_ASSERT(bits % 8u == 0u, "Size calculation is wrong.");

  return static_cast<uint32_t>(bits / 8u) + static_cast<uint32_t>(m_payload.size());
}

uint16_t CMhasPacket::calculateCRC16() const {
  static CCRC16 s_crc(0x8021u, 0xffff);

  return s_crc.calculateCRC(m_payload);
}

ilo::ByteBuffer CMhasPacket::payload() const {
  return m_payload;
}

uint32_t CMhasPacket::packetType() const {
  return m_packetType;
}

uint64_t CMhasPacket::packetLabel() const {
  return m_packetLabel;
}
