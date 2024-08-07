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

// System includes
#include <sstream>
#include <stdexcept>

// External includes
#include "ilo/bitbuffer.h"
#include "ilo/bitparser.h"

// Internal includes
#include "logging.h"
#include "mmtmhasparserlib/mhastruncationpacket.h"

using namespace mmt::mhasparserlib;

CMhasTruncationPacket::CMhasTruncationPacket(ilo::ByteBuffer::const_iterator& begin,
                                             ilo::ByteBuffer::const_iterator end)
    : CMhasPacket(begin, end) {
  ILO_ASSERT_WITH(EMhasPacketType(packetType()) == EMhasPacketType::PACTYP_AUDIOTRUNCATION,
                  std::invalid_argument, "Invalid packet type.");
  payload(m_payload.begin(), m_payload.end());
}

CMhasTruncationPacket::CMhasTruncationPacket(uint64_t label,
                                             const SMhasTruncationPacketConfig& config)
    : CMhasPacket(static_cast<uint32_t>(EMhasPacketType::PACTYP_AUDIOTRUNCATION)) {
  m_payload = {0x0, 0x0};
  applyConfig(config);
  packetLabel(label);
}

bool CMhasTruncationPacket::isActive() const {
  return m_isActive;
}

bool CMhasTruncationPacket::truncateFromBegin() const {
  return m_truncFromBegin;
}

uint16_t CMhasTruncationPacket::truncatedSamples() const {
  return m_truncSamples;
}

void CMhasTruncationPacket::setActive(bool isActive) {
  uint16_t temp = isActive ? 1 : 0;
  ilo::CBitBuffer bitBuffer(m_payload, static_cast<uint32_t>(m_payload.size() * 8));
  bitBuffer.write(temp, 1);
  m_isActive = isActive;
}

void CMhasTruncationPacket::truncateFromBegin(bool truncFromBegin) {
  uint16_t temp = truncFromBegin ? 1 : 0;
  ilo::CBitBuffer bitBuffer(m_payload, static_cast<uint32_t>(m_payload.size() * 8));
  bitBuffer.seek(2, ilo::EPosType::cur);
  bitBuffer.write(temp, 1);
  m_truncFromBegin = truncFromBegin;
}

void CMhasTruncationPacket::truncatedSamples(uint16_t truncatedSamplesLeft) {
  uint16_t temp = truncatedSamplesLeft;
  ilo::CBitBuffer bitBuffer(m_payload, static_cast<uint32_t>(m_payload.size() * 8));
  bitBuffer.seek(3, ilo::EPosType::cur);
  bitBuffer.write(temp, 13);
  m_truncSamples = truncatedSamplesLeft;
}

void CMhasTruncationPacket::payload(ilo::ByteBuffer::const_iterator begin,
                                    ilo::ByteBuffer::const_iterator end) {
  auto config = parsePayload(begin, end);
  applyConfig(config);
}

std::string CMhasTruncationPacket::packetName() const {
  return "Truncation-Packet";
}

std::string CMhasTruncationPacket::packetSpecificInfo() const {
  std::stringstream stream;

  stream << "isActive: " << m_isActive << ", truncFromBegin: " << m_truncFromBegin
         << ", nTruncSamples: " << m_truncSamples;

  return stream.str();
}

CMhasTruncationPacket::SMhasTruncationPacketConfig CMhasTruncationPacket::parsePayload(
    ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end) {
  ILO_ASSERT_WITH(end - begin == 2, std::invalid_argument, "Invalid payload size.");

  SMhasTruncationPacketConfig config;

  ilo::CBitParser parser(begin, 2u);

  config.isActive = parser.read<uint8_t>(1) == 1;

  ILO_ASSERT(parser.read<uint8_t>(1) != 1, "Reserved value doesn't match.");

  config.truncateFromBegin = parser.read<uint8_t>(1) == 1;
  config.truncatedSamples = parser.read<uint16_t>(13);

  return config;
}

void CMhasTruncationPacket::applyConfig(const SMhasTruncationPacketConfig& config) {
  setActive(config.isActive);
  truncateFromBegin(config.truncateFromBegin);
  truncatedSamples(config.truncatedSamples);
}
