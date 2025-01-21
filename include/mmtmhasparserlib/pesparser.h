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
 * @file pesparser.h
 *
 * @brief Utility MPEG PES parser structure.
 */
#pragma once

// System includes
#include <cinttypes>

// External includes
#include "ilo/common_types.h"

// Project includes
#include "version.h"
#include "mhaspacket.h"
#include "mhasparser.h"

namespace mmt {
namespace mhasparserlib {

/*!
 * Container for a MHAS access unit (a MHAS frame packets with optional non-frame MHAS packets)
 * extended with the additional information extracted from the surrounding PES packets (e.g.
 * timestamps).
 */
struct SPesMhasAccessUnit {
  static constexpr uint32_t TIMESCALE = 90000;

  explicit operator bool() const noexcept { return !packets.empty(); }

  /*!
   * The Presentation Timestamp (PTS) of the contained MHAS access unit.
   *
   * This is the value relative to the (unknown) start of the media, i.e. any 33-bit wraparound in
   * the TS timstamp field is already detected and handled accordingly.
   */
  uint64_t pts;

  /*!
   * The Decoding Timestamp (DTS) of the contained MHAS access unit.
   *
   * This is the value relative to the (unknown) start of the media, i.e. any 33-bit wraparound in
   * the TS timstamp field is already detected and handled accordingly.
   *
   * If not given explicitly in the originating PES packet, this is equal to the Presentation
   * Timestamp.
   */
  uint64_t dts;

  /*!
   * The ElementaryStreamClockReference (ESCR) of the MHAS stream.
   */
  uint64_t escr;

  /*!
   * The bitrate received at the decoder in bits/second with a granularity of 50 Bytes/s.
   *
   * This value is calculated from the Elementary Stream Rate (ES rate) of the contained PES
   * packets and may change from MHAS frame to MHAS frame.
   *
   * If the associates ES_rate field is not set in the (preceding) PES packets, this value is zero
   * meaning "not known".
   */
  uint32_t bitrate;

  /*!
   * The MHAS packets associated with the contained MHAS access unit.
   *
   * The last of the MHAS packets is the MHAS frame packet itself.
   */
  CPacketDeque packets;

  /*!
   * The audio sample rate of the MHAS stream.
   */
  uint32_t sampleRate;

  /*!
   * The duration of the contained MHAS frame packets in sampleRate ticks.
   */
  uint32_t duration;

  /*!
   * Flag indicating whether the contained MHAS frame packets are Immediate Playout Frames (IPFs).
   */
  bool isIpf;
};

/*! Packetized Elementary Stream (PES) parser and MHAS access unit extractor. */
class CPesParser {
 public:
  struct SPesPacket;
  struct SMhasConfig;

  CPesParser();
  CPesParser(const CPesParser&) = delete;
  CPesParser(CPesParser&&) noexcept = default;
  ~CPesParser() noexcept;

  CPesParser& operator=(const CPesParser&) = delete;
  CPesParser& operator=(CPesParser&&) noexcept = default;

  /*!
   * @brief Append the given binary buffer to the internal input buffer and try to parse any full
   * PES packet in the input buffer.
   *
   * To extract the MHAS access units from the PES packets, call @ref parseAccessUnits.
   */
  void feed(const ilo::ByteBuffer& vector);

  /*!
   * @brief Append the given binary buffer to the internal input buffer and try to parse any full
   * PES packet in the input buffer.
   *
   * The input range is copied internally, allowing for the given pointer to be freed/reused after
   * this function returned.
   *
   * To extract the MHAS access units from the PES packets, call @ref parseAccessUnits.
   */
  void feed(const uint8_t* rawBuffer, size_t size);

  /*!
   * @brief Add a full PES packet with the given (raw 33-bit) PTS and DTS values and the given
   * PES packet payload buffer (MHAS stream) to the input PES packet buffer.
   *
   * To extract the MHAS access units from the PES packets, call @ref parseAccessUnits.
   *
   * NOTE: This function and the @feed functions should not be mixed!
   */
  void feedPesPacket(uint64_t pts, uint64_t dts, const uint8_t* pesPayloadBuffer,
                     size_t pesPayloadSize);

  /*! Returns the number of output MHAS access units available. */
  std::size_t numAccessUnitsAvailable() const;

  /*!
   * Returns the number of bytes in the internal input buffer waiting to be parsed as part of the
   * next PES packet by successive calls to @feed.
   */
  std::size_t numBytesPending() const;

  /*!
   * Returns the number of full PES packets in the internal input buffer waiting to be parsed by
   * @ref parseAccessUnits.
   */
  std::size_t numPesPacketsPending() const;

  /*!
   * @brief Returns whether the parser is synchronized.
   *
   * @ref parseAccessUnits will drop bytes until it reaches the "synchronized" state, which is
   * achieved by parsing the first MHAS sync packet in calls to @ref parseAccessUnits.
   *
   * Once synchronized, the parser stays in that state until @ref reset is called.
   */
  bool isSynced() const;

  /*!
   * @brief Resets this parser's internal state.
   *
   * This function clears the input and output buffers and resets the "synchronized" state (see @ref
   * isSynced).
   */
  void reset();

  /*!
   * @brief Parses as many PES packets as possible from the input byte buffer and appends them to
   * the output MHAS access unit buffer.
   *
   * When called in an "unsynchronized" state (see @ref isSynced), all packets up until the first
   * MHAS sync packet will be dropped, resulting in the MHAS sync packet to be the first parsed
   * packet in the first access unit.
   *
   * The parsed access units can be retrieved by calling @ref nextAccessUnit or @ref
   * allAvailableAccessUnits
   */
  void parseAccessUnits();

  /*!
   * @brief Returns the next MHAS access unit in the output buffer or an empty object
   * (contextually conversion to bool returns "false") if there are no pending output access units.
   *
   * @ref parseAccessUnits needs to be called before output access units are available.
   */
  SPesMhasAccessUnit nextAccessUnit();

  /*!
   * @brief Returns all currently pending MHAS access units in the output buffer.
   *
   * @ref parseAccessUnits needs to be called before output access units are available.
   */
  std::deque<SPesMhasAccessUnit> allAvailableAccessUnits();

 private:
  // If the current instance is not synced, this method will search for the first PES start code
  // prefix in the given buffer. It will return an iterator pointing at the first byte of the start
  // code prefix if found. If the current instance is synced, begin is returned.
  ilo::ByteBuffer::const_iterator syncIfNecessary(ilo::ByteBuffer::const_iterator begin,
                                                  ilo::ByteBuffer::const_iterator end);

  void parseFullPesPackets();

  bool m_isSynced = false;
  CMhasParser m_mhasParser;
  ilo::ByteBuffer m_buffer;
  std::deque<std::unique_ptr<SPesPacket>> m_pesPackets;
  SPesMhasAccessUnit m_pendingAu;
  std::deque<SPesMhasAccessUnit> m_parsedAus;
  std::unique_ptr<SMhasConfig> m_streamConfig;
};
}  // namespace mhasparserlib
}  // namespace mmt
