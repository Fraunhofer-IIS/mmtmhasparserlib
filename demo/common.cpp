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
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

// External includes
#include "ilo/memory.h"
#include "mmtisobmff/reader/input.h"
#include "mmtisobmff/reader/reader.h"

// Internal includes
#include "common.h"

using namespace mmt::mhasparserlib;
using namespace mmt::isobmff;

CFileOutput::CFileOutput(const std::string& filename) : m_fstream(filename, std::ios::binary) {
  if (!m_fstream) {
    throw std::runtime_error{"Invalid file stream state"};
  }
}

void CFileOutput::writeToFile(const ilo::ByteBuffer& buffer) {
  if (!m_fstream) {
    throw std::runtime_error{"Invalid file stream state"};
  }
  m_fstream.write(reinterpret_cast<const char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
  if (!m_fstream) {
    throw std::runtime_error{"Invalid file stream state"};
  }
}

void CFileOutput::packetToBuffer(const CMhasPacket& packet, ilo::ByteBuffer& buffer) {
  buffer.resize(packet.calculatePacketSize());
  packet.writePacket(buffer);
}

void CFileOutput::packetsToBuffer(const CPacketDeque& packets, ilo::ByteBuffer& buffer) {
  buffer.clear();
  for (const CUniqueMhasPacket& packet : packets) {
    ilo::ByteBuffer temp(packet->calculatePacketSize());
    packet->writePacket(temp);
    buffer.insert(buffer.end(), temp.begin(), temp.end());
  }
}

CFileOutputRaw::CFileOutputRaw(const std::string& filename) : CFileOutput(filename) {}

void CFileOutputRaw::write(const CMhasPacket& packet) {
  ilo::ByteBuffer buffer;
  CFileOutput::packetToBuffer(packet, buffer);
  CFileOutput::writeToFile(buffer);
}

void CFileOutputRaw::write(const CPacketDeque& packets) {
  ilo::ByteBuffer buffer;
  CFileOutput::packetsToBuffer(packets, buffer);
  CFileOutput::writeToFile(buffer);
}

CFileInputMp4::CFileInputMp4(const std::string& fileName) : m_ptsOfCurrrentSample(0u) {
  m_isobmffReader = ilo::make_unique<CIsobmffReader>(ilo::make_unique<CIsobmffFileInput>(fileName));

  auto trackInfos = m_isobmffReader->trackInfos();

  if (trackInfos.empty()) {
    throw std::runtime_error{"MP4 file must contain at least one track"};
  }
  if (trackInfos[0].codec != Codec::mpegh_mhm) {
    throw std::runtime_error{"First track is not an MHM track"};
  }

  // Store track timescale since all duration are expressed with respect to this timescale
  m_trackTimescale = trackInfos[0].timescale;

  m_mpeghTrackReader = m_isobmffReader->trackByIndex<CMpeghTrackReader>(0u);
  m_mpeghTrackReader->sampleByIndex(0, m_currentSample);

  if (checkEndOfFile()) {
    throw std::runtime_error{"Retrieving first sample of the MP4 file failed."};
  }
}

CSample CFileInputMp4::currentSample() const {
  return m_currentSample;
}
uint32_t CFileInputMp4::timescale() const {
  return m_trackTimescale;
}
uint64_t CFileInputMp4::ptsOfCurrentSample() const {
  return m_ptsOfCurrrentSample;
}

bool CFileInputMp4::nextSample() {
  m_ptsOfCurrrentSample += currentSample().duration;
  m_mpeghTrackReader->nextSample(m_currentSample);
  return !checkEndOfFile();
}

bool CFileInputMp4::checkEndOfFile() const {
  return m_currentSample.rawData.empty();
}
