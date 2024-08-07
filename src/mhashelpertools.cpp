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
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

// External includes
#include "ilo/memory.h"

// Internal includes
#include "logging.h"
#include "mmtmhasparserlib/mhashelpertools.h"
#include "mmtmhasparserlib/mhasutilities.h"

using namespace mmt::mhasparserlib;

struct SPreroll {
  bool applyCrossfade = false;
  ilo::ByteBuffer config;
  std::vector<ilo::ByteBuffer> aus;
};

static uint64_t calculatePreRollSizeInBits(const SPreroll& preroll) {
  uint64_t size = 0;
  size += tools::calculateEscapedValueBitCount(preroll.config.size(), 4, 4, 8);
  size += preroll.config.size() * 8;

  size += 2;  // applyCrossfade and reserved
  size += tools::calculateEscapedValueBitCount(preroll.aus.size(), 2, 4, 0);

  for (const auto& au : preroll.aus) {
    size += tools::calculateEscapedValueBitCount(au.size(), 16, 16, 0);
    size += au.size() * 8;
  }
  return size;
}

// Extracts the preroll from the given bitparser (read position should be set to the first bit of
// the AudioPreRoll struct (Table 58 of ISO/IEC 23008-3)
static SPreroll extractPreroll(ilo::CBitParser& frameParser, size_t prerollSize = 0u) {
  SPreroll preroll{};
  uint8_t byte = 0;

  auto begin = frameParser.tell();
  auto configLen = static_cast<size_t>(readEscapedValue(frameParser, 4, 4, 8));

  if (configLen != 0) {
    preroll.config.resize(configLen);
    for (size_t currentByteIndex = 0; currentByteIndex < configLen; ++currentByteIndex) {
      byte = frameParser.read<uint8_t>(8);
      preroll.config[currentByteIndex] = byte;
    }
  }

  byte = frameParser.read<uint8_t>(2);
  preroll.applyCrossfade = (byte & 0x2u) == 0x2u;

  auto numPreRollFrames = static_cast<size_t>(readEscapedValue(frameParser, 2, 4, 0));
  preroll.aus.resize(numPreRollFrames);

  for (auto& au : preroll.aus) {
    size_t auLen = static_cast<size_t>(readEscapedValue(frameParser, 16, 16, 0));
    au.resize(auLen);

    for (size_t currentByteIndex = 0; currentByteIndex < auLen; ++currentByteIndex) {
      byte = frameParser.read<uint8_t>(8);
      au[currentByteIndex] = byte;
    }
  }

  auto end = frameParser.tell();

  if (prerollSize != 0u) {
    auto numberOfBitsRead = end - begin;
    if (numberOfBitsRead < prerollSize * 8) {
      frameParser.seek(static_cast<int32_t>(prerollSize * 8 - numberOfBitsRead),
                       ilo::EPosType::cur);
    }
  }
  return preroll;
}

static size_t writePreroll(ilo::CBitBuffer& frameWriter, const SPreroll& preroll) {
  auto begin = frameWriter.tell();
  writeEscapedValue(frameWriter, preroll.config.size(), 4, 4, 8);

  for (const auto& byte : preroll.config) {
    frameWriter.write(byte, 8);
  }

  frameWriter.write((preroll.applyCrossfade) ? 1u : 0u, 1);
  frameWriter.write(0u, 1);
  writeEscapedValue(frameWriter, preroll.aus.size(), 2, 4, 0);

  for (const auto& au : preroll.aus) {
    writeEscapedValue(frameWriter, au.size(), 16, 16, 0);
    for (const auto& byte : au) {
      frameWriter.write(byte, 8);
    }
  }

  auto bits = frameWriter.tell() - begin;
  // Extension payload needs to be byte aligned. We cannot use the byteAlign method because the
  // frame itself is most probably not byte aligned.
  if (bits % 8 != 0) {
    frameWriter.seek(8 - static_cast<int32_t>(bits) % 8, ilo::EPosType::cur);
  }

  return (bits + 7) / 8;
}

static size_t readPayloadLength(ilo::CBitParser& frameParser) {
  auto payloadLength = frameParser.read<uint32_t>(8);
  if (payloadLength == 255u) {
    auto temp = frameParser.read<uint16_t>(16);
    payloadLength += temp - 2;
  }
  return payloadLength;
}

static void writeFlagsAndPayloadLength(ilo::CBitBuffer& writer, uint64_t payloadLength) {
  writer.write(0x06u, 3);
  if (payloadLength >= 255u) {
    writer.write(255u, 8);
    writer.write(payloadLength - 253u, 16);
  } else {
    writer.write(payloadLength, 8);
  }
}

static void copyPayload(ilo::CBitParser& source, ilo::CBitBuffer& dest) {
  while (!source.eof()) {
    uint8_t bitsToRead =
        static_cast<uint8_t>(std::min(8u, source.nofBits() - source.nofReadBits()));

    auto value = source.read<uint8_t>(bitsToRead);
    dest.write(value, bitsToRead);
  }
}

CPacketDeque tools::readNextFrame(ilo::ByteBuffer::const_iterator& begin,
                                  ilo::ByteBuffer::const_iterator end) {
  if (begin == end) {
    return CPacketDeque{};
  }
  ILO_ASSERT(begin < end, "Invalid range provided");

  const int32_t DEFAULT_READ_SIZE = 512;
  auto currentFeedPosition = begin;
  auto currentOutputPosition = begin;

  CMhasParser mhasParser;

  bool frameFound = false;

  CPacketDeque returnQueue;
  while (!frameFound) {
    auto bytesToRead = (end - currentFeedPosition) >= DEFAULT_READ_SIZE ? DEFAULT_READ_SIZE
                                                                        : end - currentFeedPosition;
    if (bytesToRead <= 0) {
      break;
    }
    mhasParser.sync();
    mhasParser.feed(&currentFeedPosition[0], static_cast<std::size_t>(bytesToRead));
    currentFeedPosition += bytesToRead;
    mhasParser.parsePackets();

    auto allPackets = mhasParser.allAvailablePackets();
    for (auto& packet : allPackets) {
      currentOutputPosition += packet->calculatePacketSize();
      returnQueue.push_back(std::move(packet));
      if (returnQueue.back()->packetType() == (uint32_t)EMhasPacketType::PACTYP_MPEGH3DAFRAME) {
        frameFound = true;
        break;
      }
    }
  }

  if (!frameFound) {
    return CPacketDeque{};
  }

  begin = currentOutputPosition;
  return returnQueue;
}

void tools::embedConfigurationIntoPreRoll(CMhasFramePacket& frame,
                                          const ilo::ByteBuffer& mpegh3daConfig) {
  ILO_ASSERT_WITH(frame.isIPF(), std::invalid_argument,
                  "Provided frame does not contain a preroll");

  // Parse Preroll as defined in ISO/IEC 23008-3 Table 58
  auto currentFrame = frame.payload();
  embedConfigurationIntoPreRoll(currentFrame, mpegh3daConfig);
  frame.payload(currentFrame.begin(), currentFrame.end());
}

void tools::embedConfigurationIntoPreRoll(ilo::ByteBuffer& au,
                                          const ilo::ByteBuffer& mpegh3daConfig) {
  ilo::CBitParser auParser(au);

  // Read usacIndependencyFlag (1), usacExtElementPresent (1) and usacExtElmentUseDefaultLength = 0
  auto value = auParser.read<uint8_t>(3);
  ILO_ASSERT(value == 0x06u, "Provided frame is not an IPF");

  // usacExtElementPayloadLenght
  auto payloadLength = readPayloadLength(auParser);
  auto preroll = extractPreroll(auParser, payloadLength);
  ILO_ASSERT(preroll.config.empty(), "The provided IPF already contains a configuration");
  preroll.config = mpegh3daConfig;
  auto calculatedPrerollSize = (calculatePreRollSizeInBits(preroll) + 7) / 8;

  uint64_t finalSizeInBit =
      3 /*indep and so on*/ + ((calculatedPrerollSize >= 255) ? 24 : 8) + calculatedPrerollSize * 8;
  finalSizeInBit += (auParser.nofBits() - auParser.nofReadBits());
  uint32_t finalSizeInBytes = static_cast<uint32_t>(finalSizeInBit + 7) / 8;

  ilo::ByteBuffer finalBuffer(finalSizeInBytes, 0);
  ilo::CBitBuffer auWriter(finalBuffer, static_cast<uint32_t>(finalBuffer.size() * 8));
  writeFlagsAndPayloadLength(auWriter, calculatedPrerollSize);
  writePreroll(auWriter, preroll);
  copyPayload(auParser, auWriter);

  ILO_ASSERT(finalSizeInBit == auWriter.tell(), "Preallocation failed.");

  au = std::move(finalBuffer);
}

CPacketDeque::const_iterator tools::findPacketWithType(const CPacketDeque& packetDequeue,
                                                       EMhasPacketType type) {
  return std::find_if(packetDequeue.begin(), packetDequeue.end(),
                      [type](const CUniqueMhasPacket& packet) {
                        return packet->packetType() == static_cast<uint32_t>(type);
                      });
}

void tools::writePacketsToByteBuffer(const CPacketDeque& packetDeque, ilo::ByteBuffer& buffer) {
  auto size = std::accumulate(packetDeque.begin(), packetDeque.end(), uint32_t{0},
                              [](uint32_t sum, const std::unique_ptr<CMhasPacket>& packet) {
                                return sum + packet->calculatePacketSize();
                              });

  buffer.resize(size);
  auto begin = buffer.begin();
  for (const auto& packet : packetDeque) {
    auto end = begin + packet->calculatePacketSize();

    ILO_ASSERT(
        end <= buffer.end(),
        "Unable to write packet to buffer, there seems to be an error in calculate packet size.");
    packet->writePacket(begin, end);
    begin = end;
  }
}

uint64_t tools::calculateEscapedValueBitCount(uint64_t value, uint32_t first, uint32_t second,
                                              uint32_t third) {
  uint64_t bits = first;
  if (value >= (2u << (first - 1)) - 1) {
    bits += second;
    value -= (2u << (first - 1)) - 1;
    if (value >= (2u << (second - 1)) - 1) {
      bits += third;
    }
  }

  return bits;
}

void parseAndCopySpeakerConfig3d(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer) {
  // speakerLayoutType
  auto speakerLayoutType = bitParser.read<uint32_t>(2);
  bitBuffer.write(speakerLayoutType, 2);
  // CICPspeakerLayoutIdx
  if (speakerLayoutType == 0) {
    bitBuffer.write(bitParser.read<uint8_t>(6), 6);
  } else {
    auto numSpeakers = static_cast<uint32_t>(readEscapedValue(bitParser, 5, 8, 16) + 1);
    writeEscapedValue(bitBuffer, numSpeakers - 1, 5, 8, 16);

    switch (speakerLayoutType) {
      case 1:
        for (uint32_t i = 0; i < numSpeakers; ++i) {
          // CICPspeakerIdx
          bitBuffer.write(bitParser.read<uint8_t>(7), 7);
        }

        break;

      case 2: {
        // mpegh3daFlexibleSpeakerConfig(numSpeakers)

        bool angularPrecision = bitParser.read<uint32_t>(1) == 1u;
        bitBuffer.write(angularPrecision);

        for (uint32_t i = 0; i < numSpeakers; ++i) {
          // mpegh3daSpeakerDescription()

          bool isCICPspeakerIdx = bitParser.read<uint8_t>(1) == 1u;
          bitBuffer.write(isCICPspeakerIdx);

          // CICPspeakerIdx
          if (isCICPspeakerIdx) {
            bitBuffer.write(bitParser.read<uint8_t>(7), 7);
          } else {
            auto ElevationClass = bitParser.read<uint8_t>(2);
            bitBuffer.write(ElevationClass, 2);

            if (ElevationClass == 3) {
              uint8_t ElevationAngleIdx = 0;
              if (angularPrecision == 0) {
                ElevationAngleIdx = bitParser.read<uint8_t>(5);
                bitBuffer.write(ElevationAngleIdx, 5);
              } else {
                ElevationAngleIdx = bitParser.read<uint8_t>(7);
                bitBuffer.write(ElevationAngleIdx, 7);
              }

              if (ElevationAngleIdx != 0) {
                bool ElevationDirection = bitParser.read<uint8_t>(1) == 1u;
                bitBuffer.write(ElevationDirection);
              }
            }

            uint8_t AzimuthAngleIdx = 0;
            bool azimuthDirectionFlag = false;
            if (angularPrecision == 0) {
              AzimuthAngleIdx = bitParser.read<uint8_t>(6);
              bitBuffer.write(AzimuthAngleIdx, 6);

              azimuthDirectionFlag = AzimuthAngleIdx != 0 && AzimuthAngleIdx != 36;
            } else {
              AzimuthAngleIdx = bitParser.read<uint8_t>(8);
              bitBuffer.write(AzimuthAngleIdx, 8);

              azimuthDirectionFlag = AzimuthAngleIdx != 0 && AzimuthAngleIdx != 180;
            }

            if (azimuthDirectionFlag) {
              bool AzimuthDirection = bitParser.read<uint8_t>(1) == 1u;
              bitBuffer.write(AzimuthDirection);
            }

            bool isLFE = bitParser.read<uint8_t>(1) == 1u;
            bitBuffer.write(isLFE);
          }
        }  // for (uint32_t i = 0; i < numSpeakers; ++i)

        break;
      }
      default:
        throw std::runtime_error("Wrong speakerLayoutType found in mpegh3daConfig");
    }
  }
}

void parseAndCopyFrameworkConfig3d(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer,
                                   uint32_t& numberOfSignals) {
  numberOfSignals = 0;

  auto bsNumSignalGroups = bitParser.read<uint32_t>(5);
  bitBuffer.write(bsNumSignalGroups, 5);

  for (uint32_t grp = 0; grp < bsNumSignalGroups + 1; ++grp) {
    auto signalGroupType = bitParser.read<uint32_t>(3);
    bitBuffer.write(signalGroupType, 3);

    auto bsNumberOfSignals = static_cast<uint32_t>(readEscapedValue(bitParser, 5, 8, 16));
    writeEscapedValue(bitBuffer, bsNumberOfSignals, 5, 8, 16);

    numberOfSignals += bsNumberOfSignals + 1;

    switch (signalGroupType) {
      case 0:  // SignalGroupTypeChannels
      case 2:  // SignalGroupTypeSAOC
      {
        // differsFromReferenceLayout[grp] or saocDmxLayoutPresent
        bool flag = bitParser.read<uint32_t>(1) == 1u;
        bitBuffer.write(flag);

        if (flag == 1) {
          parseAndCopySpeakerConfig3d(bitParser, bitBuffer);
        }

        break;
      }
      case 1:  // SignalGroupTypeObject
      case 3:  // SignalGroupTypeHOA
        break;

      default:
        throw std::runtime_error("Wrong signalGroupType found in mpegh3daConfig");
    }
  }
}

void parseAndCopyMpegh3daCoreConfig(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer,
                                    uint32_t& enhancedNoiseFilling) {
  // tw_mdct + fullbandLpd + noiseFilling
  auto flags = bitParser.read<uint32_t>(3);
  bitBuffer.write(flags, 3);

  enhancedNoiseFilling = bitParser.read<uint32_t>(1);
  bitBuffer.write(enhancedNoiseFilling, 1);

  if (enhancedNoiseFilling) {
    // igfUseEnf + igfUseHighRes + igfUseWhitening + igfAfterTnsSynth + igfStartIndex + igfStopIndex
    auto value = bitParser.read<uint32_t>(13);
    bitBuffer.write(value, 13);
  }
}

void parseAndCopySbrConfig(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer) {
  // harmonicSBR + bs_interTes + bs_pvc
  auto value = bitParser.read<uint32_t>(3);
  bitBuffer.write(value, 3);

  // dflt_start_freq + dflt_stop_freq
  value = bitParser.read<uint32_t>(8);
  bitBuffer.write(value, 8);

  bool dflt_header_extra1 = bitParser.read<uint32_t>(1) == 1u;
  bitBuffer.write(dflt_header_extra1);

  bool dflt_header_extra2 = bitParser.read<uint32_t>(1) == 1u;
  bitBuffer.write(dflt_header_extra2);

  if (dflt_header_extra1) {
    // dflt_freq_scale + dflt_alter_scale + dflt_noise_bands
    value = bitParser.read<uint32_t>(5);
    bitBuffer.write(value, 5);
  }

  if (dflt_header_extra2) {
    // dflt_limiter_bands + dflt_limiter_gains + dflt_interpol_freq + dflt_smoothing_mode
    value = bitParser.read<uint32_t>(6);
    bitBuffer.write(value, 6);
  }
}

void parseAndCopyMps212Config(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer,
                              uint32_t stereoConfigIndex) {
  // bsFreqRes + bsFixedGainDMX
  auto value = bitParser.read<uint32_t>(6);
  bitBuffer.write(value, 6);

  // bsTempShapeConfig
  auto bsTempShapeConfig = bitParser.read<uint32_t>(2);
  bitBuffer.write(bsTempShapeConfig, 2);

  // bsDecorrConfig + bsHighRateMode + bsPhaseCoding
  value = bitParser.read<uint32_t>(4);
  bitBuffer.write(value, 4);

  bool bsOttBandsPhasePresent = bitParser.read<uint32_t>(1) == 1u;
  bitBuffer.write(bsOttBandsPhasePresent);

  if (bsOttBandsPhasePresent) {
    auto bsOttBandsPhase = bitParser.read<uint32_t>(5);
    bitBuffer.write(bsOttBandsPhase, 5);
  }

  bool bsResidualCoding = stereoConfigIndex > 1;
  if (bsResidualCoding) {
    // bsResidualBands + bsPseudoLr
    value = bitParser.read<uint32_t>(6);
    bitBuffer.write(value, 6);
  }

  if (bsTempShapeConfig == 2) {
    bool bsEnvQuantMode = bitParser.read<uint32_t>(1) == 1u;
    bitBuffer.write(bsEnvQuantMode);
  }
}

void parseAndCopyMpegh3daDecoderConfig(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer,
                                       uint32_t coreSbrFrameLengthIndex, uint32_t numberOfSignals) {
  uint32_t enhancedNoiseFilling = 0;

  // Compute sbrRatioIndex from coreSbrFrameLengthIndex
  static const std::map<uint32_t /* coreSbrFrameLengthIndex */, uint32_t /* sbrRatioIndex */>
      sbrRatioIndexMap{{0, 0}, {1, 0}, {2, 2}, {3, 3}, {4, 1}};

  uint32_t sbrRatioIndex = sbrRatioIndexMap.at(coreSbrFrameLengthIndex);

  uint8_t numOfBits =
      static_cast<uint8_t>(static_cast<uint8_t>(log(numberOfSignals - 1) / log(2)) + 1);

  auto numElements = static_cast<uint32_t>(readEscapedValue(bitParser, 4, 8, 16) + 1);
  writeEscapedValue(bitBuffer, numElements - 1, 4, 8, 16);

  bool elementLengthPresent = bitParser.read<uint32_t>(1) == 1u;
  bitBuffer.write(elementLengthPresent);

  for (uint32_t elemIdx = 0; elemIdx < numElements; ++elemIdx) {
    auto usacElementType = bitParser.read<uint32_t>(2);
    bitBuffer.write(usacElementType, 2);

    switch (usacElementType) {
      case 0:  // ID_USAC_SCE
        // mpegh3daSingleChannelElementConfig(sbrRatioIndex)
        parseAndCopyMpegh3daCoreConfig(bitParser, bitBuffer, enhancedNoiseFilling);
        if (sbrRatioIndex > 0) {
          // SbrConfig()
          parseAndCopySbrConfig(bitParser, bitBuffer);
        }
        break;

      case 1:  // ID_USAC_CPE
      {
        // mpegh3daChannelPairElementConfig(sbrRatioIndex)
        parseAndCopyMpegh3daCoreConfig(bitParser, bitBuffer, enhancedNoiseFilling);
        if (enhancedNoiseFilling) {
          bool igfIndependentTiling = bitParser.read<uint32_t>(1) == 1u;
          bitBuffer.write(igfIndependentTiling);
        }
        uint32_t stereoConfigIndex = 0;
        if (sbrRatioIndex > 0) {
          // SbrConfig()
          parseAndCopySbrConfig(bitParser, bitBuffer);
          stereoConfigIndex = bitParser.read<uint32_t>(2);
          bitBuffer.write(stereoConfigIndex, 2);
        } else {
          stereoConfigIndex = 0;
        }

        if (stereoConfigIndex > 0) {
          // Mps212Config(stereoConfigIndex)
          parseAndCopyMps212Config(bitParser, bitBuffer, stereoConfigIndex);
        }

        // qceIndex
        auto qceIndex = bitParser.read<uint32_t>(2);
        bitBuffer.write(qceIndex, 2);

        if (qceIndex > 0) {
          bool shiftIndex0 = bitParser.read<uint32_t>(1) == 1u;
          bitBuffer.write(shiftIndex0);

          if (shiftIndex0) {
            auto shiftChannel0 = bitParser.read<uint32_t>(numOfBits);
            bitBuffer.write(shiftChannel0, numOfBits);
          }
        }

        bool shiftIndex1 = bitParser.read<uint32_t>(1) == 1u;
        bitBuffer.write(shiftIndex1);
        if (shiftIndex1) {
          auto shiftChannel1 = bitParser.read<uint32_t>(numOfBits);
          bitBuffer.write(shiftChannel1, numOfBits);
        }

        if (sbrRatioIndex == 0 && qceIndex == 0) {
          bool lpdStereoIndex = bitParser.read<uint32_t>(1) == 1u;
          bitBuffer.write(lpdStereoIndex);
        }
        break;
      }
      case 2:  // ID_USAC_LFE
        break;

      case 3:  // ID_USAC_EXT
      {
        auto usacExtElementType = static_cast<uint32_t>(readEscapedValue(bitParser, 4, 8, 16));
        writeEscapedValue(bitBuffer, usacExtElementType, 4, 8, 16);

        // usacExtElementConfigLength
        auto usacExtElementConfigLength =
            static_cast<uint32_t>(readEscapedValue(bitParser, 4, 8, 16));
        writeEscapedValue(bitBuffer, usacExtElementConfigLength, 4, 8, 16);

        bool usacExtElementDefaultLengthPresent = bitParser.read<uint32_t>(1) == 1u;
        bitBuffer.write(usacExtElementDefaultLengthPresent);

        if (usacExtElementDefaultLengthPresent) {
          auto usacExtElementDefaultLength =
              static_cast<uint32_t>(readEscapedValue(bitParser, 8, 16, 0));
          writeEscapedValue(bitBuffer, usacExtElementDefaultLength, 8, 16, 0);
        }

        bool usacExtElementPayloadFrag = bitParser.read<uint32_t>(1) == 1u;
        bitBuffer.write(usacExtElementPayloadFrag);

        // Copying usacExtElement bytes
        for (uint32_t i = 0; i < usacExtElementConfigLength; ++i) {
          auto value = bitParser.read<uint32_t>(8);
          bitBuffer.write(value, 8);
        }

        break;
      }
    }  // switch(usacElementType)
  }
}

void parseAndCopyMpegh3daConfigExtension(ilo::CBitParser& bitParser, ilo::CBitBuffer& bitBuffer,
                                         uint32_t usacConfigExtensionPresent,
                                         const ilo::ByteBuffer& mae_AudioSceneInfo) {
  if (usacConfigExtensionPresent == 0) {
    // If no extension was already present we have to create one for the ASI
    writeEscapedValue(bitBuffer, 0 /* numConfigExtensions will be + 1 */, 2, 4, 8);
  } else {
    // If extension(s) was(were) already present we have to insert the one for the ASI
    uint32_t numConfigExtensions = static_cast<uint32_t>(readEscapedValue(bitParser, 2, 4, 8)) + 1;
    writeEscapedValue(bitBuffer, numConfigExtensions - 1 + 1 /* +1 for the ext we add */, 2, 4, 8);

    // Copy the available extensions
    for (uint32_t confExtIdx = 0; confExtIdx < numConfigExtensions; ++confExtIdx) {
      auto usacConfigExtType = static_cast<uint32_t>(readEscapedValue(bitParser, 4, 8, 16));
      writeEscapedValue(bitBuffer, usacConfigExtType, 4, 8, 16);

      ILO_ASSERT(usacConfigExtType != 3 /* ID_CONFIG_EXT_AUDIOSCENE_INFO */,
                 "One ASI extension already present in mpegh3daConfig");

      auto usacConfigExtLength = static_cast<uint32_t>(readEscapedValue(bitParser, 4, 8, 16));
      writeEscapedValue(bitBuffer, usacConfigExtLength, 4, 8, 16);

      // Copying usacConfigExt bytes
      for (uint32_t i = 0; i < usacConfigExtLength; ++i) {
        auto value = bitParser.read<uint32_t>(8);
        bitBuffer.write(value, 8);
      }
    }
  }

  // Insert the ASI extension

  // usacConfigExtType
  writeEscapedValue(bitBuffer, 3 /* ID_CONFIG_EXT_AUDIOSCENE_INFO */, 4, 8, 16);
  // usacConfigExtLength
  writeEscapedValue(bitBuffer, mae_AudioSceneInfo.size(), 4, 8, 16);
  // Copying the ASI bytes
  for (auto info : mae_AudioSceneInfo) {
    bitBuffer.write(info, 8);
  }
}

ilo::CUniqueBuffer tools::insertAsiInConfig(const ilo::ByteBuffer& mpegh3daConfig,
                                            const ilo::ByteBuffer& mae_AudioSceneInfo) {
  auto begin = mpegh3daConfig.cbegin();
  auto end = mpegh3daConfig.cend();

  ILO_ASSERT_WITH(begin < end, std::invalid_argument, "Invalid iterators provided (begin >= end)");

  ilo::CBitParser bitParser(begin, end);
  ilo::CBitBuffer bitBuffer(
      static_cast<uint32_t>(mpegh3daConfig.size() + mae_AudioSceneInfo.size()));

  auto mpegh3daProfileLevelIndication = bitParser.read<uint32_t>(8);
  bitBuffer.write(mpegh3daProfileLevelIndication, 8);

  auto usacSamplingFrequencyIndex = bitParser.read<uint32_t>(5);
  bitBuffer.write(usacSamplingFrequencyIndex, 5);

  if (usacSamplingFrequencyIndex == 0x1Fu) {
    auto usacSamplingFrequency = bitParser.read<uint32_t>(24);
    bitBuffer.write(usacSamplingFrequency, 24);
  }

  // coreSbrFrameLengthIndex
  auto coreSbrFrameLengthIndex = bitParser.read<uint32_t>(3);
  bitBuffer.write(coreSbrFrameLengthIndex, 3);
  ILO_ASSERT(coreSbrFrameLengthIndex <= 4,
             "Invalid coreSbrFrameLengthIndex found in mpegh3daConfig");

  // reserved + receiverDelayCompensation
  auto value = bitParser.read<uint32_t>(2);
  bitBuffer.write(value, 2);

  uint32_t numberOfSignals = 0;
  // SpeakerConfig3d()
  parseAndCopySpeakerConfig3d(bitParser, bitBuffer);
  // FrameworkConfig3d()
  parseAndCopyFrameworkConfig3d(bitParser, bitBuffer, numberOfSignals);
  // mpegh3daDecoderConfig()
  parseAndCopyMpegh3daDecoderConfig(bitParser, bitBuffer, coreSbrFrameLengthIndex, numberOfSignals);

  // usacConfigExtensionPresent
  auto usacConfigExtensionPresent = bitParser.read<uint32_t>(1);
  // The extension is always present
  bitBuffer.write(1u, 1);
  // Parse / Create an mpegh3daConfigExtension()
  parseAndCopyMpegh3daConfigExtension(bitParser, bitBuffer, usacConfigExtensionPresent,
                                      mae_AudioSceneInfo);

  // Allocate new buffer from bitBuffer and return it
  bitBuffer.byteAlign();
  auto result = bitBuffer.bytebuffer();
  return ilo::make_unique<ilo::ByteBuffer>(result.begin(), result.end());
}

tools::SBitstreamConfig tools::extractSampleRateAndFrameSize(
    const CMhasConfigPacket& configPacket) {
  CMhasConfigPacket::SConfig configPacketInfo = configPacket.mhasConfigInfo();
  ILO_ASSERT(configPacket.isLcProfile(), "Only LC bitstreams are supported.");
  ILO_ASSERT(configPacketInfo.outputSamplingFrequency != -1,
             "Unable to extract output sample rate.");
  ILO_ASSERT(configPacketInfo.outputFramesize != -1, "Unable to extract frame size.");

  SBitstreamConfig bitstreamConfig;
  bitstreamConfig.outputSampleRate =
      static_cast<uint32_t>(configPacketInfo.outputSamplingFrequency);
  bitstreamConfig.frameSize = static_cast<uint32_t>(configPacketInfo.outputFramesize);

  return bitstreamConfig;
}
