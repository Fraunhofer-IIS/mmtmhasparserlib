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
/*!
 * @file mhaspacketizer.h
 *
 * @brief MHAS packetizer structure.
 */
#pragma once

// System includes
#include <cinttypes>
#include <map>

// External includes
#include "ilo/common_types.h"

// Project includes
#include "version.h"
#include "mhaspacket.h"
#include "mhasparser.h"

namespace mmt {
namespace mhasparserlib {

/*!
 * Container for a MHAS access unit, a grouping of at least one MHAS frame packet with optional
 * non-frame MHAS packets.
 *
 * The access unit may contain multiple streams, in which case it will hold one MHAS frame packet
 * per audio stream in the input MHAS stream.
 */
struct SMhasAccessUnit {
  /*!
   * The MHAS packets associated with the MHAS access unit.
   *
   * For single-stream, the last of the MHAS packets is the MHAS frame packet itself.
   */
  CPacketDeque packets;

  /*!
   * The audio sample rate of the MHAS stream.
   */
  uint32_t sampleRate;

  /*!
   * The duration of the contained MHAS frame packets in the corresponding sample rate.
   *
   * This value already takes into account any associated truncation packets.
   */
  uint32_t duration;

  /*!
   * Flag indicating whether the contained MHAS frame packets are Immediate Playout Frames (IPFs).
   */
  bool isIpf;

  explicit operator bool() const noexcept { return !packets.empty(); }

  /*!
   * Splits the packets in this MHAS access unit according to their MPEG-H 3DA streams (main,
   * secondary, etc.) they belong to and returns a single MHAS access unit structure per stream.
   *
   * The relative order of the packets associated with an MPEG-H 3DA stream is preserved.
   *
   * MHAS packets applicable to all MPEG-H 3DA streams (i.e. packets with a label of zero) are
   * duplicated for all streams.
   *
   * NOTE: Other MHAS packets not associated with any MPEG-H 3DA streams (i.e. packets with a label
   * different from all MPEG-H 3DA streams) are dropped!
   */
  std::vector<SMhasAccessUnit> splitStreams() &&;
};

/*!
 * MHAS packetizer to parse an input MHAS stream and group the MHAS packets to access units.
 */
class CMhasPacketizer {
 public:
  struct SStreamConfig;

  struct SConfig {
    /*!
     * Throw exceptions on certain input errors.
     *
     * Controls behavior on how to handle certain invalid/unhandled input MHAS packets.
     * If this flag is set (true), throws exceptions aborting further processing.
     * If this flag is cleared (false), prints logging messages and tries to continue processing
     * (e.g. discarding the packets that cannot be processed).
     *
     * E.g. for MHAS frame packets present in the input before the first MHAS config packet, this
     * flag decides whether an error is thrown (true) or the MHAS frame packets are discarded and
     * processing continues (false).
     */
    bool strictMode = true;
  };

  CMhasPacketizer(const SConfig& config);
  CMhasPacketizer(const CMhasPacketizer&) = delete;
  CMhasPacketizer(CMhasPacketizer&&) noexcept = default;
  ~CMhasPacketizer() noexcept;

  CMhasPacketizer& operator=(const CMhasPacketizer&) = delete;
  CMhasPacketizer& operator=(CMhasPacketizer&&) noexcept = default;

  /*! Returns the number of output MHAS access units available. */
  uint32_t numAccessUnitsAvailable() const;

  /*!
   * Returns the number of bytes in the internal input buffer waiting to be parsed by @ref
   * parsePackets.
   */
  uint32_t numBytesPending() const;

  /*!
   * Append the given binary buffer to the internal input buffer to be parsed on the next call to
   * @ref parseAccessUnits.
   *
   * @see CMhasParser::feed.
   */
  void feed(const ilo::ByteBuffer& vector);

  /*!
   * @brief Append the given binary buffer to the internal input buffer to be parsed on the next
   * call to @ref parseAccessUnits.
   *
   * The input range is copied internally, allowing for the given pointer to be freed/reused after
   * this function returns.
   *
   * @see CMhasParser::feed.
   */
  void feed(const uint8_t* rawBuffer, size_t size);

  /*!
   * @brief Returns whether the packetizer is synchronized.
   *
   * @see CMhasParser::isSynced.
   *
   * Once synchronized, the packetizer stays in that state until @ref reset is called.
   */
  bool isSynced() const;

  /*!
   * @brief Mark the packetizer as "synchronized".
   *
   * @see CMhasParser::sync.
   */
  void sync();

  /*!
   * @brief Resets this packetizer's internal state.
   *
   * This function clears the input and output buffers and resets the "synchronized" state (see @ref
   * isSynced).
   *
   * @see CMhasParser::reset.
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
   * The fully parsed access units can be retrieved by calling @ref nextAccessUnit or @ref
   * allAvailableAccessUnits.
   *
   * @see CMhasParser::parsePackets.
   */
  void parseAccessUnits();

  /*!
   * @brief Returns the next full MHAS access unit in the output packet buffer or an empty object
   * (contextually conversion to bool returns "false") if there are no pending full output access
   * units.
   *
   * @ref parseAccessUnits needs to be called before output packets are available.
   */
  SMhasAccessUnit nextAccessUnit();

  /*!
   * @brief Returns all currently pending full MHAS access units in the output buffer.
   *
   * @ref parseAccessUnits needs to be called before output packets are available.
   */
  std::deque<SMhasAccessUnit> allAvailableAccessUnits();

 private:
  SConfig m_config;
  CMhasParser m_parser;
  CPacketDeque m_pendingPackets;
  std::map<uint64_t, SStreamConfig> m_configs;
  std::deque<SMhasAccessUnit> m_parsedAus;
};
}  // namespace mhasparserlib
}  // namespace mmt
