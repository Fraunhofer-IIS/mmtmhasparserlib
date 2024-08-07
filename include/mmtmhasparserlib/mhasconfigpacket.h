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
 * @file mhasconfigpacket.h
 *
 * @brief MHAS config packet definitions
 */
#pragma once

// System includes
#include <map>
#include <memory>
#include <string>
#include <vector>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "version.h"
#include "mhaspacket.h"
#include "mhasasipacket.h"

namespace mmt {
namespace mhasparserlib {
/*!
 * @brief Definition of an MHAS configuration packet.
 */
class CMhasConfigPacket final : public CMhasPacket {
 public:
  /*!
   * @brief Initialize the MHAS config packet by reading the given byte range.
   *
   * The given byte range must contain exactly a single MHAS config packet.
   *
   * The begin iterator is incremented by the number of bytes read to parse this MHAS packet.
   */
  CMhasConfigPacket(ilo::ByteBuffer::const_iterator& begin, ilo::ByteBuffer::const_iterator end);

  /*!
   * @brief Initialize the MHAS config packet by reading the given byte range and overwrite the @ref
   * packetLabel.
   *
   * The given byte range must contain exactly a single MHAS config packet.
   */
  CMhasConfigPacket(uint64_t label, ilo::ByteBuffer::const_iterator payloadStart,
                    ilo::ByteBuffer::const_iterator payloadEnd);

  /*!
   * @brief Sets the payload buffer to the given byte range.
   *
   * The given byte range must contain exactly a single MHAS config packet.
   *
   * The given range is parsed and the packet representation of this object is updated with the
   * contents of the buffer.
   */
  void payload(ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end) override;
  using CMhasPacket::payload;

  //! Returns whether this MHAS config packet has a Low Complexity (LC) profile
  bool isLcProfile() const;

  //! Representation of the SpeakerConfig3d() structure as defined in ISO/IEC 23008-3
  //! subclause 5.2.2.2
  struct SSpeakerConfig3d {
    //! The speaker layout type as defined in ISO/IEC 23008-3 subclause 5.3.3
    enum class ESpeakerLayoutType : uint8_t {
      INVALID = 255,
      CICPSPEAKERLAYOUTIDX = 0,
      CICPSPEAKERIDX = 1,
      FLEXIBLESPEAKERCONFIG = 2,
      CONTRIBUTIONMODE = 3
    };

    //! The type of speaker layout
    ESpeakerLayoutType speakerLayoutType = ESpeakerLayoutType::INVALID;

    //! The number of speakers for non-zero @ref speakerLayoutType.
    uint32_t numSpeakers = 0u;

    /*!
     * @brief The channel configuration as defined in ISO/IEC 23091-3.
     *
     * @note This field is only set if @ref speakerLayoutType is set to
     * ESpeakerLayoutType::CICPSPEAKERLAYOUTIDX.
     */
    uint8_t cicpSpeakerLayoutIdx = 255u;

    /*!
     * @brief The loudspeaker geometry as defined in ISO/IEC 23091-3.
     *
     * @note This field is only set if @ref speakerLayoutType is non-zero.
     */
    std::vector<uint8_t> cicpSpeakerIdx;

    bool operator==(const SSpeakerConfig3d& compare) const;
    bool operator!=(const SSpeakerConfig3d& compare) const;
  };

  //! Container for signal and signal group configuration values
  struct SSignals3d {
    //! Representation of a single signal group
    struct SSignalGroup {
      //! The type of this signal group
      enum class ESignalGroupType : uint8_t {
        INVALID = 255,
        CHANNELS = 0,
        OBJECT = 1,
        // SAOC = 2, // we currently do not support SAOC
        HOA = 3
      };

      //! The associated element IDs depending on the signal group type.
      std::vector<uint8_t> metaDataElementIds;

      //! The ID of the group
      uint8_t idx = 255;
      //! The type indicator of the signal group.
      ESignalGroupType signalGroupType = ESignalGroupType::INVALID;

      //! The speaker configuration for this signal group
      std::shared_ptr<SSpeakerConfig3d> audioChannelLayout;

      //! The number of signals in this signal group.
      uint32_t numSignals = 0;
    };

    //! Representation of a single audio signal
    struct SSignal {
      //! Referenced signal group
      std::shared_ptr<SSignalGroup> signalGroup;
      //! Index of the referenced signal in the signal group
      uint8_t signalNumber = 255u;
    };

    //! The total number of higher order ambisonics (HOA) transport channels in all signal groups.
    uint32_t numHoaTransportChannel = 0u;
    //! The total number of audio objects in all signal groups.
    uint32_t numAudioObjects = 0u;
    //! The total number of audio channels in all signal groups.
    uint32_t numAudioChannels = 0u;
    //! The signal groups contained in this configuration.
    std::vector<std::shared_ptr<SSignalGroup>> signalGroups;
    //! Map from metadata element ID to signal
    std::map<uint8_t, SSignal> signals;

    /*!
     * @brief Updates this signal group with data extracted from the given ASI.
     *
     * @note Only the first call to this function has any effect, i.e. fields updated from a
     * previous ASI will not be overwritten.
     */
    void applyAsi(const CMhasAsiPacket::SAudioSceneInfo& audioSceneInfo);

   private:
    bool asiApplied = false;
    uint8_t appliedMetaDataElementIdOffset = 0;
  };

  //! Representation of the mpegh3daConfig() structure as defined in ISO/IEC 23008-3
  //! subclause 5.2.2.1
  struct SConfig {
    /*!
     * @brief The MPEG-H 3D Audio profile and level information, according to ISO/IEC 23008-3
     * subclause 5.3.2
     *
     * @see ISO/IEC 23008-3 subclause 5.3
     */
    uint8_t profileLevelIndication = 0u;

    /*!
     * @brief The index into the USAC sampling frequency mapping, as defined in ISO/IEC 23003-3
     * subclause 6.
     *
     * @see ISO/IEC 23008-3 subclause 5.3
     */
    uint8_t samplingFrequencyIndex = 0u;

    /*!
     * @brief The index into the SBR and output frame length mapping, as defined in ISO/IEC 23003-3
     * subclause 6
     *
     * @see ISO/IEC 23008-3 subclause 5.3
     */
    uint8_t coreSbrFrameLengthIndex = 0u;

    //! The effective output frame size in samples.
    int32_t outputFramesize = -1;
    //! The effective output sampling frequency in Hz.
    int32_t outputSamplingFrequency = -1;
    //! The effective sampling frequency in Hz.
    int32_t samplingFrequency = -1;

    //! The signals in this configuration
    SSignals3d signals3d;
    //! The (optional) reference layout
    std::shared_ptr<SSpeakerConfig3d> referenceLayout;

    //! Flag indicating whether audio pre-roll is present
    bool audioPreRollPresent = false;
    //! The list of compatible profile level indications
    std::vector<uint8_t> compatibleProfileLevels;

    /*!
     * @brief Parses the given byte range and sets the object's fields accordingly
     *
     * The begin iterator is incremented by the number of bytes read to parse this structure.
     */
    void parsePayload(ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end);

    //! Updates this configuration object with data extracted from the given ASI
    void applyAsi(const CMhasAsiPacket::SAudioSceneInfo& audioSceneInfo);
  };

  //! Returns the configuration structure inside this packet
  SConfig mhasConfigInfo() const;

 protected:
  //! Returns the name of this MHAS packet type
  std::string packetName() const override;

 private:
  SConfig m_config;
};
}  // namespace mhasparserlib
}  // namespace mmt
