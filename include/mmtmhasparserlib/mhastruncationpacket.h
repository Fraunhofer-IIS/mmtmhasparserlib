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
 * @file mhastruncationpacket.h
 *
 * @brief MHAS truncation packet definitions
 */
#pragma once

// System includes
#include <string>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "version.h"
#include "mhaspacket.h"

namespace mmt {
namespace mhasparserlib {
//! Representation of a MHAS truncation packet
class CMhasTruncationPacket final : public CMhasPacket {
 public:
  //! Representation of the MHAS audioTruncationInfo() structure as defined in ISO/IEC 23008-3
  //! subclause 14.2.2.
  struct SMhasTruncationPacketConfig {
    //! Whether this truncation message is active or the decoder should ignore it.
    bool isActive = false;
    //! Whether the truncation happens from the beginning (true) or the end (false) of the packet.
    bool truncateFromBegin = true;
    //! The number of audio samples to truncate.
    uint16_t truncatedSamples = 0u;
  };

  /*!
   * @brief Initialize the truncation packet by reading the given byte range.
   *
   * The given byte range must contain exactly a single MHAS truncation packet.
   *
   * The begin iterator is incremented by the number of bytes read to parse this MHAS packet.
   */
  CMhasTruncationPacket(ilo::ByteBuffer::const_iterator& begin,
                        ilo::ByteBuffer::const_iterator end);
  //! Initializes the truncation packet with the given packet label and truncation configuration.
  CMhasTruncationPacket(uint64_t label, const SMhasTruncationPacketConfig& config);

  //! Returns whether this truncation message is active or the decoder should ignore it.
  bool isActive() const;
  //! Returns whether the truncation happens from the beginning (true) or the end (false) of the
  //! packet.
  bool truncateFromBegin() const;
  //! Returns the number of audio samples to truncate.
  uint16_t truncatedSamples() const;

  //! Sets the active (see @ref isActive) flag of this truncation.
  void setActive(bool isActive);
  //! Sets whether the truncation is located at the beginning (true) or end (false) of the packet.
  void truncateFromBegin(bool truncFromBegin);
  //! Sets the number of truncated samples.
  void truncatedSamples(uint16_t truncatedSamples);

  /*!
   * @brief Sets the payload buffer to the given byte range.
   *
   * The given byte range must contain exactly a single MHAS truncation packet.
   *
   * The given range is parsed and the packet representation of this object is updated with the
   * contents of the buffer.
   */
  void payload(ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end) override;

 protected:
  //! Returns the name of this MHAS packet type
  std::string packetName() const override;

  //! Returns additional information about this packet
  std::string packetSpecificInfo() const override;

 private:
  SMhasTruncationPacketConfig parsePayload(ilo::ByteBuffer::const_iterator begin,
                                           ilo::ByteBuffer::const_iterator end);
  void applyConfig(const SMhasTruncationPacketConfig& config);

  bool m_isActive = false;
  bool m_truncFromBegin = false;
  uint16_t m_truncSamples = 0;
};
}  // namespace mhasparserlib
}  // namespace mmt
