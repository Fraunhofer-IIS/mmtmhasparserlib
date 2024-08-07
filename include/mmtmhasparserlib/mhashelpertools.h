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

/*!
 * @file mhashelpertools.h
 *
 * @brief MHAS helper tools
 */
#pragma once

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "version.h"
#include "mhasparser.h"
#include "mhasframepacket.h"
#include "mhasconfigpacket.h"

namespace mmt {
namespace mhasparserlib {
namespace tools {

//! Configuration parameters for a MHAS config packet
struct SBitstreamConfig {
  //! The output sample rate in samples per second (Hz).
  uint32_t outputSampleRate = 0;
  //! The frame size in number of samples per frame.
  uint32_t frameSize = 0;
};

//! Calculates how many bits a required to express the given value as escaped value as defined in
//! ISO/IEC 23003-3:2012, 5.2, Table 16.
uint64_t calculateEscapedValueBitCount(uint64_t value, uint32_t first, uint32_t second,
                                       uint32_t third);

//! Inserts the mae_AudioSceneInfo() struct into the extension payload of the mpegh3daConfig()
//! struct.
ilo::CUniqueBuffer insertAsiInConfig(const ilo::ByteBuffer& mpegh3daConfig,
                                     const ilo::ByteBuffer& mae_AudioSceneInfo);

/*!
 * @brief Reads all packets belonging to a frame from a given buffer.
 *
 * In the single stream use-case, the returned packets should be exactly the payload of one mhm
 * track sample.
 *
 * The begin iterator is incremented by the number of bytes read to parse a single full frame.
 */
CPacketDeque readNextFrame(ilo::ByteBuffer::const_iterator& begin,
                           ilo::ByteBuffer::const_iterator end);

//! Embeds a configuration into the AUDIO_PRE_ROLL of an given mhas packet.
void embedConfigurationIntoPreRoll(CMhasFramePacket& frame, const ilo::ByteBuffer& mpegh3daConfig);

//! Embeds a configuration into the AUDIO_PRE_ROLL of an given raw frame.
void embedConfigurationIntoPreRoll(ilo::ByteBuffer& au, const ilo::ByteBuffer& mpegh3daConfig);

//! Finds the first occurrence of a specified packet type in the provided deque.
CPacketDeque::const_iterator findPacketWithType(const CPacketDeque& packetDequeue,
                                                EMhasPacketType type);

//! Writes a given packet deque to a given buffer (data in given buffer will be overwritten).
void writePacketsToByteBuffer(const CPacketDeque& packetDeque, ilo::ByteBuffer& buffer);

//! Extracts the output sampling rate and frame size (in samples) from the MHAS config packet.
SBitstreamConfig extractSampleRateAndFrameSize(const CMhasConfigPacket& configPacket);
}  // namespace tools
}  // namespace mhasparserlib
}  // namespace mmt
