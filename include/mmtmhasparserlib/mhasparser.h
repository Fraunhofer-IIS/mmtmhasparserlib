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
 * @file mhasparser.h
 *
 * @brief Main MHAS parser structure.
 */
#pragma once

// System includes
#include <cinttypes>

// External includes
#include "ilo/common_types.h"

// Project includes
#include "version.h"
#include "mhaspacket.h"

namespace mmt {
namespace mhasparserlib {
//! Main MHAS parser.
class CMhasParser {
 public:
  //! Append the given binary buffer to the internal input buffer to be parsed on the next call to
  //! @ref parsePackets.
  void feed(const ilo::ByteBuffer& vector);
  /*!
   * @brief Append the given binary buffer to the internal input buffer to be parsed on the next
   * call to @ref parsePackets.
   *
   * The input range is copied internally, allowing for the given pointer to be freed/reused after
   * this function returned.
   */
  void feed(const uint8_t* rawBuffer, size_t size);

  //! Returns the number of output MHAS packets available.
  uint32_t numPacketsAvailable() const;
  //! Returns the number of bytes in the internal input buffer waiting to be parsed by @ref
  //! parsePackets.
  uint32_t numBytesPending() const;

  /*!
   * @brief Returns whether the parser is synchronized.
   *
   * @ref parsePackets will drop all MHAS packets until it reaches the "synchronized" state, which
   * can be achieved either by explicitly calling @ref sync or by parsing the first MHAS sync packet
   * in calls to @ref parsePackets.
   *
   * Once synchronized, the parser stays in that state until @ref reset is called.
   */
  bool isSynced() const;

  /*!
   * @brief Mark the parser as "synchronized".
   *
   * See @ref isSynced for the purpose of the "synchronized" flag.
   */
  void sync();

  /*!
   * @brief Resets this parser's internal state.
   *
   * This function clears the input and output buffers and resets the "synchronized" state (see @ref
   * isSynced).
   */
  void reset();

  /*!
   * @brief Parses as many MHAS packets as possible from the input byte buffer and appends them to
   * the output packet buffer.
   *
   * When called in an "unsynchronized" state (see @ref isSynced), all packets up until the first
   * MHAS sync packet will be dropped, resulting in the MHAS sync packet to be the first parsed
   * packet. Immediate return of MHAS packets can be activated by calling @ref sync before calling
   * this function.
   *
   * The parsed packets can be retrieved by calling @ref nextPacket or @ref allAvailablePackets.
   */
  void parsePackets();

  /*!
   * @brief Returns the next MHAS packet in the output packet buffer or NULL if there are no pending
   * output packets.
   *
   * @ref parsePackets needs to be called before output packets are available.
   */
  CUniqueMhasPacket nextPacket();

  /*!
   * @brief Returns all currently pending MHAS packets in the output buffer.
   *
   * @ref parsePackets needs to be called before output packets are available.
   */
  CPacketDeque allAvailablePackets();

 private:
  // If the current instance is not synced, this method will search for a sync packet in the given
  // buffer. It will update the begin iterator to point at the first byte of the sync packet if
  // found. Otherwise, begin will point to end.
  bool syncIfNecessary(ilo::ByteBuffer::const_iterator& begin, ilo::ByteBuffer::const_iterator end);

  bool m_isSynced = false;
  ilo::ByteBuffer m_buffer;
  CPacketDeque m_parsedPackets;
  bool m_audioPreRollPresent = false;
};
}  // namespace mhasparserlib
}  // namespace mmt
