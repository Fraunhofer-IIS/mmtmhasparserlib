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
 * @file mhaspacket.h
 *
 * @brief MHAS base packet types
 */
#pragma once

// System includes
#include <deque>
#include <map>
#include <memory>
#include <string>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "version.h"

namespace mmt {
namespace mhasparserlib {
class CMhasPacket;
//! Type alias to an unique pointer of the MHAS Packet type
using CUniqueMhasPacket = std::unique_ptr<CMhasPacket>;

//! Type alias to a deque (bidirectional queue) of MHAS Packets
using CPacketDeque = std::deque<CUniqueMhasPacket>;

/*!
 * @brief Supported MHAS packet types, as defined in ISO/IEC 23008-3 subsection 14.3
 */
enum class EMhasPacketType : uint32_t {
  /* ISO space */
  PACTYP_FILLDATA = 0,
  PACTYP_MPEGH3DACFG = 1,
  PACTYP_MPEGH3DAFRAME = 2,
  PACTYP_AUDIOSCENEINFO = 3,
  PACTYP_SYNC = 6,
  PACTYP_MARKER = 8,
  PACTYP_CRC16 = 9,
  PACTYP_AUDIOTRUNCATION = 17,
  /* not ISO space */
  PACTYP_FRAMELENGTH = 129,
};

//! Defines the order of MHAS packets for IPFs (as defined in ISO/IEC 23008-3 2nd Ed. Clause 20.6)
extern const std::map<EMhasPacketType, uint32_t> IPF_PACKETS_ORDER;

//! Returns a string representation (name) for supported MHAS packet types
std::string packetTypeToString(EMhasPacketType packetType);

/*!
 * @brief Base class for all supported MHAS packet types, as defined in ISO/IEC 23008-3 section 14.
 *
 * This class contains definitions and base implementations for common functionality across all
 * packet types.
 */
class CMhasPacket {
 public:
  /*!
   * @brief Initialize the MHAS packet by reading the given byte range
   *
   * The begin iterator is incremented by the number of bytes read to parse this MHAS packet.
   */
  CMhasPacket(ilo::ByteBuffer::const_iterator& begin, ilo::ByteBuffer::const_iterator end);
  virtual ~CMhasPacket() noexcept = default;

  /*!
   * @brief Parses a single MHAS packet from the given byte range.
   *
   * The begin iterator is incremented by the number of bytes read to parse the first MHAS packet.
   *
   * @return the parsed MHAS packet representation of the appropriate child-type or NULL.
   */
  static CUniqueMhasPacket s_parseNextPacket(ilo::ByteBuffer::const_iterator& begin,
                                             ilo::ByteBuffer::const_iterator end,
                                             bool audioPreRollPresent);

  /*!
   * @brief Sets the payload buffer to the given byte range.
   *
   * @note The given payload is not parsed and the packet representation of this object is not
   * updated.
   */
  virtual void payload(ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end);

  /*!
   * @brief Sets the label of this packet to the given value.
   */
  virtual void packetLabel(uint64_t label);

  //! Returns a copy to the internal payload buffer.
  virtual ilo::ByteBuffer payload() const;

  /*!
   * @see EMhasPacketType
   * @returns this packet's type.
   */
  uint32_t packetType() const;

  //! Returns this packet's label.
  uint64_t packetLabel() const;

  //! Returns the total space in bytes this packet (header + payload) requires.
  uint32_t calculatePacketSize() const;

  //! Returns the CRC16 checksum of this packet's payload.
  uint16_t calculateCRC16() const;

  /*!
   * @param [in] dumpPayload - if set, also includes the payload bytes in the returned string.
   * @returns a string representation of this packet.
   */
  virtual std::string toString(bool dumpPayload = true);

  //! Writes this packet's payload to the given buffer, overwriting any previous content.
  virtual void writePacket(ilo::ByteBuffer& vector) const;

  /*!
   * @brief Writes this packet's payload to the given byte range.
   *
   * @note This function throws exceptions if the byte range size does not match the packet size.
   * @see calculatePacketSize
   */
  virtual void writePacket(ilo::ByteBuffer::iterator begin, ilo::ByteBuffer::iterator end) const;

  /*!
   * @brief Writes this packet's payload to the given byte range.
   *
   * @note This function throws exceptions if the byte range size does not match the packet size.
   * @see calculatePacketSize
   *
   * @returns the number of bytes written, equals to the packet's size.
   */
  virtual std::size_t writePacket(uint8_t* rawBuffer, std::size_t rawBufferSize) const;

 protected:
  //! Packet type should only be set once, since it can change the runtime type
  explicit CMhasPacket(uint32_t packetType);

  //! Returns the name of this MHAS packet type
  virtual std::string packetName() const { return "Mhas-Packet"; }

  //! Returns additional information about this packet
  virtual std::string packetSpecificInfo() const { return ""; }

 protected:
  //! The raw payload buffer of this packet
  ilo::ByteBuffer m_payload;
  //! The packet label
  uint64_t m_packetLabel;

 private:
  uint32_t m_packetType;
};
}  // namespace mhasparserlib
}  // namespace mmt
