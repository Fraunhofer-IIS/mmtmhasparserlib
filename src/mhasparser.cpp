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

// Internal includes
#include "mmtmhasparserlib/mhasparser.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"

using namespace mmt::mhasparserlib;

void CMhasParser::feed(const ilo::ByteBuffer& vector) {
  m_buffer.insert(m_buffer.end(), vector.begin(), vector.end());
}

void CMhasParser::feed(const uint8_t* rawBuffer, size_t size) {
  m_buffer.insert(m_buffer.end(), rawBuffer, rawBuffer + size);
}

uint32_t CMhasParser::numPacketsAvailable() const {
  return static_cast<uint32_t>(m_parsedPackets.size());
}

uint32_t CMhasParser::numBytesPending() const {
  return static_cast<uint32_t>(m_buffer.size());
}

void CMhasParser::sync() {
  m_isSynced = true;
}

bool CMhasParser::isSynced() const {
  return m_isSynced;
}

void CMhasParser::reset() {
  m_buffer.clear();
  m_parsedPackets.clear();
  m_isSynced = false;
}

void CMhasParser::parsePackets() {
  auto readIterator = m_buffer.cbegin();

  if (!syncIfNecessary(readIterator, m_buffer.end())) {
    auto endIterator = m_buffer.begin() + (readIterator - m_buffer.cbegin());
    m_buffer.erase(m_buffer.begin(), endIterator);
    return;
  }

  while (auto packet =
             CMhasPacket::s_parseNextPacket(readIterator, m_buffer.end(), m_audioPreRollPresent)) {
    if (packet->packetType() == static_cast<uint32_t>(EMhasPacketType::PACTYP_MPEGH3DACFG)) {
      m_audioPreRollPresent =
          dynamic_cast<CMhasConfigPacket*>(packet.get())->mhasConfigInfo().audioPreRollPresent;
    }
    m_parsedPackets.push_back(std::move(packet));
  }

  m_buffer.erase(m_buffer.begin(), readIterator);
}

CUniqueMhasPacket CMhasParser::nextPacket() {
  if (!m_parsedPackets.empty()) {
    auto packet = std::move(m_parsedPackets.front());
    m_parsedPackets.pop_front();
    return packet;
  }
  return nullptr;
}

CPacketDeque CMhasParser::allAvailablePackets() {
  CPacketDeque deque;
  deque.swap(m_parsedPackets);
  return deque;
}

bool CMhasParser::syncIfNecessary(ilo::ByteBuffer::const_iterator& begin,
                                  ilo::ByteBuffer::const_iterator end) {
  while (!m_isSynced && end - begin > 3) {
    if (begin[0] == 0xC0u && begin[1] == 0x01u && begin[2] == 0xA5u) {
      m_isSynced = true;
      return true;
    }

    ++begin;
  }

  return m_isSynced;
}
