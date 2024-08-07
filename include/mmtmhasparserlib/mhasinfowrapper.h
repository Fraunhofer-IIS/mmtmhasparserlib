/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2020 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * @file mhasinfowrapper.h
 *
 * @brief helper to combine MHAS info structs
 */
#pragma once

// System includes
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Internal includes
#include "version.h"
#include "mhasparser.h"
#include "mhasasipacket.h"
#include "mhasconfigpacket.h"

namespace mmt {
namespace mhasparserlib {
//! Helper class to extract and combine MHAS info from different MHAS packets.
class CMhasInfoWrapper {
 public:
  //! A single language-specific item description.
  struct SItemDescription {
    //! The ISO 639-2 language code.
    std::array<char, 3> lang = {'u', 'n', 'd'};

    //! The UTF-8 description text.
    std::string desc;
  };

  //! A single signal group
  struct SGroup {
    //! The unique group ID.
    uint8_t id = 0;
    //! The list of language-specific group descriptions.
    std::vector<SItemDescription> descriptions;
    //! The signals that are part of this group.
    std::vector<CMhasConfigPacket::SSignals3d::SSignal> signals;
  };

  //! A preset of signal groups
  struct SGroupPreset {
    //! Reference to a signal group
    struct SGroupReference {
      //! The type of the group reference
      enum class EGroupType { Invalid, Group, SwitchGroup };

      //! The ID of the referenced element (group or switch group).
      uint8_t referenceId = 0;
      //! The type of the referenced element.
      EGroupType groupType = EGroupType::Invalid;
      //! Whether the referenced metadata element is turned on or off.
      bool onOff = false;
    };

    //! The unique preset ID.
    uint8_t id = 0;
    //! The list of language-specific group preset descriptions.
    std::vector<SItemDescription> descriptions;
    //! Referenced groups or switch groups.
    std::vector<SGroupReference> groupIds;
  };

  //! A single switch group
  struct SSwitchGroup {
    //! The unique switch group ID.
    uint8_t id = 0;
    //! The list of language-specific switch group descriptions.
    std::vector<SItemDescription> descriptions;
    //! The ID of the default group (which will be activated initially upon activation of this
    //! switch group).
    uint8_t defaultGroupId = 0;
    //! Group IDs of all referenced groups contained in this switch group.
    std::vector<uint8_t> groupIds;
  };

  //! Collected information on a MHAS buffer data
  struct SMhasBufferInfo {
    //! The reference layout for which the content was created (if available).
    std::shared_ptr<CMhasConfigPacket::SSpeakerConfig3d> referenceLayout;
    //! Mapping of group IDs to the group information.
    std::map<uint8_t, SGroup> groups;
    //! Mapping of switch group IDs to their switch group info.
    std::map<uint8_t, SSwitchGroup> switchGroups;
    //! Mapping of group preset IDs to group preset info.
    std::map<uint8_t, SGroupPreset> groupPresets;
    //! The signal groups contained in the MHAS stream.
    std::vector<std::shared_ptr<CMhasConfigPacket::SSignals3d::SSignalGroup>> signalGroups;
    //! Flag indicating whether a bitstream error occurred and the parser needed to be reset since
    //! the last time getMhasInfo was called.
    bool wasResynced = false;
  };

  /*!
   * @brief Feed byte buffer to MHAS parser wrapper.
   *
   * This method will automatically restart the parser if a bitstream error occurs.
   * This method can throw exceptions if inconsistencies in the parsed data are detected.
   */
  void feed(const std::vector<uint8_t>& buffer);

  /*!
   * @brief Check whether information of the current MHAS stream is available.
   *
   * The returned value will be false before reading the first config/ASI and directly after
   * bitstream errors.
   */
  bool isMhasInfoAvailable() const;

  /*!
   * @brief Return the current MHAS information from the wrapper.
   *
   * This will throw an exception if @ref isMhasInfoAvailable returns false.
   */
  SMhasBufferInfo getMhasInfo();

 private:
  CMhasParser m_mhasParser;
  SMhasBufferInfo m_mhasBufferInfo;
  CMhasConfigPacket::SSignals3d m_signals3d;

  bool m_isMhasInfoAvailable = false;
  bool m_seekingAsi = false;

  void handleParsedPackets();
  void extractAsiInfo(const CMhasAsiPacket& mhasAsiPacket);
  void initGroups(const CMhasAsiPacket::SAudioSceneInfo& audioScene);
  void initSwitchGroups(const CMhasAsiPacket::SAudioSceneInfo& audioScene);
  void initGroupPresets(const CMhasAsiPacket::SAudioSceneInfo& audioScene,
                        std::vector<uint8_t>& groupPresetIds);
  void handleGroupPresetExtension(const CMhasAsiPacket::SAudioSceneGrpPresetEx& presetExt,
                                  const std::vector<uint8_t>& groupPresetIds);
  void handleGroupDescription(const CMhasAsiPacket::SAudioSceneDescription& audioSceneDescription,
                              const CMhasAsiPacket::SAudioSceneDataElement::EDataType& dataType);
};
}  // namespace mhasparserlib
}  // namespace mmt
