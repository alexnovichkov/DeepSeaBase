/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "juce_AudioChannelSet.h"

namespace {
    int findNextSetBit(std::bitset<1024> channels, int index)
    {
        for (int i=index; i<1024; ++i) {
            if (channels.test(i)) return i;
        }
        return -1;
    }
    void setBitRange(std::bitset<1024> &channels, int index, int count)
    {
        Q_ASSERT(index+count<1024);
        for (int i=0; i<count; ++i) {
            channels.set(index+i);
        }
    }
    int getHighestBit(std::bitset<1024> channels)
    {
        for (int i=31; i>=0; ++i) {
            if (channels.test(i)) return i;
        }
        return -1;
    }
}

namespace juce
{



AudioChannelSet::AudioChannelSet (quint32 c) : channels (static_cast<qint64> (c))
{
}

AudioChannelSet::AudioChannelSet (const QList<ChannelType>& c)
{
    for (auto channel : c)
        addChannel (channel);
}

bool AudioChannelSet::operator== (const AudioChannelSet& other) const noexcept  { return channels == other.channels; }
bool AudioChannelSet::operator!= (const AudioChannelSet& other) const noexcept  { return channels != other.channels; }
//bool AudioChannelSet::operator<  (const AudioChannelSet& other) const noexcept  { return channels <  other.channels; }

QString AudioChannelSet::getChannelTypeName (AudioChannelSet::ChannelType type)
{
    if (type >= discreteChannel0)
        return "Discrete " + QString::number(type - discreteChannel0 + 1);

    switch (type)
    {
        case left:                return ("Left");
        case right:               return ("Right");
        case centre:              return ("Centre");
        case LFE:                 return ("LFE");
        case leftSurround:        return ("Left Surround");
        case rightSurround:       return ("Right Surround");
        case leftCentre:          return ("Left Centre");
        case rightCentre:         return ("Right Centre");
        case centreSurround:      return ("Centre Surround");
        case leftSurroundRear:    return ("Left Surround Rear");
        case rightSurroundRear:   return ("Right Surround Rear");
        case topMiddle:           return ("Top Middle");
        case topFrontLeft:        return ("Top Front Left");
        case topFrontCentre:      return ("Top Front Centre");
        case topFrontRight:       return ("Top Front Right");
        case topRearLeft:         return ("Top Rear Left");
        case topRearCentre:       return ("Top Rear Centre");
        case topRearRight:        return ("Top Rear Right");
        case wideLeft:            return ("Wide Left");
        case wideRight:           return ("Wide Right");
        case LFE2:                return ("LFE 2");
        case leftSurroundSide:    return ("Left Surround Side");
        case rightSurroundSide:   return ("Right Surround Side");
        case ambisonicW:          return ("Ambisonic W");
        case ambisonicX:          return ("Ambisonic X");
        case ambisonicY:          return ("Ambisonic Y");
        case ambisonicZ:          return ("Ambisonic Z");
        case topSideLeft:         return ("Top Side Left");
        case topSideRight:        return ("Top Side Right");
        case ambisonicACN4:       return ("Ambisonic 4");
        case ambisonicACN5:       return ("Ambisonic 5");
        case ambisonicACN6:       return ("Ambisonic 6");
        case ambisonicACN7:       return ("Ambisonic 7");
        case ambisonicACN8:       return ("Ambisonic 8");
        case ambisonicACN9:       return ("Ambisonic 9");
        case ambisonicACN10:      return ("Ambisonic 10");
        case ambisonicACN11:      return ("Ambisonic 11");
        case ambisonicACN12:      return ("Ambisonic 12");
        case ambisonicACN13:      return ("Ambisonic 13");
        case ambisonicACN14:      return ("Ambisonic 14");
        case ambisonicACN15:      return ("Ambisonic 15");
        case bottomFrontLeft:     return ("Bottom Front Left");
        case bottomFrontCentre:   return ("Bottom Front Centre");
        case bottomFrontRight:    return ("Bottom Front Right");
        case bottomSideLeft:      return ("Bottom Side Left");
        case bottomSideRight:     return ("Bottom Side Right");
        case bottomRearLeft:      return ("Bottom Rear Left");
        case bottomRearCentre:    return ("Bottom Rear Centre");
        case bottomRearRight:     return ("Bottom Rear Right");
        case discreteChannel0:    return ("Discrete channel");
        default:                  break;
    }

    return "Unknown";
}

QString AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::ChannelType type)
{
    if (type >= discreteChannel0)
        return QString::number(type - discreteChannel0 + 1);

    switch (type)
    {
        case left:                return "L";
        case right:               return "R";
        case centre:              return "C";
        case LFE:                 return "Lfe";
        case leftSurround:        return "Ls";
        case rightSurround:       return "Rs";
        case leftCentre:          return "Lc";
        case rightCentre:         return "Rc";
        case centreSurround:      return "Cs";
        case leftSurroundRear:    return "Lrs";
        case rightSurroundRear:   return "Rrs";
        case topMiddle:           return "Tm";
        case topFrontLeft:        return "Tfl";
        case topFrontCentre:      return "Tfc";
        case topFrontRight:       return "Tfr";
        case topRearLeft:         return "Trl";
        case topRearCentre:       return "Trc";
        case topRearRight:        return "Trr";
        case wideLeft:            return "Wl";
        case wideRight:           return "Wr";
        case LFE2:                return "Lfe2";
        case leftSurroundSide:    return "Lss";
        case rightSurroundSide:   return "Rss";
        case ambisonicACN0:       return "ACN0";
        case ambisonicACN1:       return "ACN1";
        case ambisonicACN2:       return "ACN2";
        case ambisonicACN3:       return "ACN3";
        case ambisonicACN4:       return "ACN4";
        case ambisonicACN5:       return "ACN5";
        case ambisonicACN6:       return "ACN6";
        case ambisonicACN7:       return "ACN7";
        case ambisonicACN8:       return "ACN8";
        case ambisonicACN9:       return "ACN9";
        case ambisonicACN10:      return "ACN10";
        case ambisonicACN11:      return "ACN11";
        case ambisonicACN12:      return "ACN12";
        case ambisonicACN13:      return "ACN13";
        case ambisonicACN14:      return "ACN14";
        case ambisonicACN15:      return "ACN15";
        case topSideLeft:         return "Tsl";
        case topSideRight:        return "Tsr";
        case bottomFrontLeft:     return "Bfl";
        case bottomFrontCentre:   return "Bfc";
        case bottomFrontRight:    return "Bfr";
        case bottomSideLeft:      return "Bsl";
        case bottomSideRight:     return "Bsr";
        case bottomRearLeft:      return "Brl";
        case bottomRearCentre:    return "Brc";
        case bottomRearRight:     return "Brr";
        default:                  break;
    }

    if (type >= ambisonicACN4 && type <= ambisonicACN35)
        return "ACN" + QString::number (type - ambisonicACN4 + 4);

    return {};
}

AudioChannelSet::ChannelType AudioChannelSet::getChannelTypeFromAbbreviation (const QString& abbr)
{
    if (abbr.length() > 0 && (abbr[0] >= '0' && abbr[0] <= '9'))
        return static_cast<AudioChannelSet::ChannelType> (static_cast<int> (discreteChannel0)
                                                               + abbr.toInt() - 1);

    if (abbr == "L")     return left;
    if (abbr == "R")     return right;
    if (abbr == "C")     return centre;
    if (abbr == "Lfe")   return LFE;
    if (abbr == "Ls")    return leftSurround;
    if (abbr == "Rs")    return rightSurround;
    if (abbr == "Lc")    return leftCentre;
    if (abbr == "Rc")    return rightCentre;
    if (abbr == "Cs")    return centreSurround;
    if (abbr == "Lrs")   return leftSurroundRear;
    if (abbr == "Rrs")   return rightSurroundRear;
    if (abbr == "Tm")    return topMiddle;
    if (abbr == "Tfl")   return topFrontLeft;
    if (abbr == "Tfc")   return topFrontCentre;
    if (abbr == "Tfr")   return topFrontRight;
    if (abbr == "Trl")   return topRearLeft;
    if (abbr == "Trc")   return topRearCentre;
    if (abbr == "Trr")   return topRearRight;
    if (abbr == "Wl")    return wideLeft;
    if (abbr == "Wr")    return wideRight;
    if (abbr == "Lfe2")  return LFE2;
    if (abbr == "Lss")   return leftSurroundSide;
    if (abbr == "Rss")   return rightSurroundSide;
    if (abbr == "W")     return ambisonicW;
    if (abbr == "X")     return ambisonicX;
    if (abbr == "Y")     return ambisonicY;
    if (abbr == "Z")     return ambisonicZ;
    if (abbr == "ACN0")  return ambisonicACN0;
    if (abbr == "ACN1")  return ambisonicACN1;
    if (abbr == "ACN2")  return ambisonicACN2;
    if (abbr == "ACN3")  return ambisonicACN3;
    if (abbr == "ACN4")  return ambisonicACN4;
    if (abbr == "ACN5")  return ambisonicACN5;
    if (abbr == "ACN6")  return ambisonicACN6;
    if (abbr == "ACN7")  return ambisonicACN7;
    if (abbr == "ACN8")  return ambisonicACN8;
    if (abbr == "ACN9")  return ambisonicACN9;
    if (abbr == "ACN10") return ambisonicACN10;
    if (abbr == "ACN11") return ambisonicACN11;
    if (abbr == "ACN12") return ambisonicACN12;
    if (abbr == "ACN13") return ambisonicACN13;
    if (abbr == "ACN14") return ambisonicACN14;
    if (abbr == "ACN15") return ambisonicACN15;
    if (abbr == "Tsl")   return topSideLeft;
    if (abbr == "Tsr")   return topSideRight;
    if (abbr == "Bfl")   return bottomFrontLeft;
    if (abbr == "Bfc")   return bottomFrontCentre;
    if (abbr == "Bfr")   return bottomFrontRight;
    if (abbr == "Bsl")   return bottomSideLeft;
    if (abbr == "Bsr")   return bottomSideRight;
    if (abbr == "Brl")   return bottomRearLeft;
    if (abbr == "Brc")   return bottomRearCentre;
    if (abbr == "Brr")   return bottomRearRight;
    return unknown;
}

QString AudioChannelSet::getSpeakerArrangementAsString() const
{
    QStringList speakerTypes;

    for (auto& speaker : getChannelTypes())
    {
        auto name = getAbbreviatedChannelTypeName (speaker);

        if (!name.isEmpty())
            speakerTypes.append (name);
    }

    return speakerTypes.join (" ");
}

AudioChannelSet AudioChannelSet::fromAbbreviatedString (const QString& str)
{
    AudioChannelSet set;

    for (auto& abbr : str.split(" "))
    {
        auto type = getChannelTypeFromAbbreviation (abbr);

        if (type != unknown)
            set.addChannel (type);
    }

    return set;
}

QString AudioChannelSet::getDescription() const
{
    if (isDiscreteLayout())            return "Discrete #" + QString::number(size());
    if (*this == disabled())           return "Disabled";
    if (*this == mono())               return "Mono";
    if (*this == stereo())             return "Stereo";

    if (*this == createLCR())          return "LCR";
    if (*this == createLRS())          return "LRS";
    if (*this == createLCRS())         return "LCRS";

    if (*this == create5point0())       return "5.0 Surround";
    if (*this == create5point1())       return "5.1 Surround";
    if (*this == create6point0())       return "6.0 Surround";
    if (*this == create6point1())       return "6.1 Surround";
    if (*this == create6point0Music())  return "6.0 (Music) Surround";
    if (*this == create6point1Music())  return "6.1 (Music) Surround";
    if (*this == create7point0())       return "7.0 Surround";
    if (*this == create7point1())       return "7.1 Surround";
    if (*this == create7point0SDDS())   return "7.0 Surround SDDS";
    if (*this == create7point1SDDS())   return "7.1 Surround SDDS";
    if (*this == create7point0point2()) return "7.0.2 Surround";
    if (*this == create7point1point2()) return "7.1.2 Surround";

    if (*this == quadraphonic())       return "Quadraphonic";
    if (*this == pentagonal())         return "Pentagonal";
    if (*this == hexagonal())          return "Hexagonal";
    if (*this == octagonal())          return "Octagonal";

    return "Unknown";
}

bool AudioChannelSet::isDiscreteLayout() const noexcept
{
    for (auto& speaker : getChannelTypes())
        if (speaker <= ambisonicACN35)
            return false;

    return true;
}

int AudioChannelSet::size() const noexcept
{
    return channels.count();
}

AudioChannelSet::ChannelType AudioChannelSet::getTypeOfChannel (int index) const noexcept
{
    int bit = findNextSetBit(channels, 0);

    for (int i = 0; i < index && bit >= 0; ++i)
        bit = findNextSetBit (channels, bit + 1);

    return static_cast<ChannelType> (bit);
}

int AudioChannelSet::getChannelIndexForType (AudioChannelSet::ChannelType type) const noexcept
{
    int idx = 0;

    for (int bit = findNextSetBit (channels, 0); bit >= 0; bit = findNextSetBit (channels, bit + 1))
    {
        if (static_cast<ChannelType> (bit) == type)
            return idx;

        idx++;
    }

    return -1;
}

QList<AudioChannelSet::ChannelType> AudioChannelSet::getChannelTypes() const
{
    QList<ChannelType> result;

    for (int bit = findNextSetBit(channels, 0); bit >= 0; bit = findNextSetBit (channels, bit + 1))
        result.append (static_cast<ChannelType> (bit));

    return result;
}

void AudioChannelSet::addChannel (ChannelType newChannel)
{
    const int bit = static_cast<int> (newChannel);
    Q_ASSERT (bit >= 0 && bit < 1024);
    channels.set(bit);
}

void AudioChannelSet::removeChannel (ChannelType newChannel)
{
    const int bit = static_cast<int> (newChannel);
    Q_ASSERT (bit >= 0 && bit < 1024);
    channels.reset (bit);
}

AudioChannelSet AudioChannelSet::disabled()            { return {}; }
AudioChannelSet AudioChannelSet::mono()                { return AudioChannelSet (1u << centre); }
AudioChannelSet AudioChannelSet::stereo()              { return AudioChannelSet ((1u << left) | (1u << right)); }
AudioChannelSet AudioChannelSet::createLCR()           { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre)); }
AudioChannelSet AudioChannelSet::createLRS()           { return AudioChannelSet ((1u << left) | (1u << right) | (1u << surround)); }
AudioChannelSet AudioChannelSet::createLCRS()          { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << surround)); }
AudioChannelSet AudioChannelSet::create5point0()       { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurround) | (1u << rightSurround)); }
AudioChannelSet AudioChannelSet::create5point1()       { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << LFE) | (1u << leftSurround) | (1u << rightSurround)); }
AudioChannelSet AudioChannelSet::create6point0()       { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurround) | (1u << rightSurround) | (1u << centreSurround)); }
AudioChannelSet AudioChannelSet::create6point1()       { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << LFE) | (1u << leftSurround) | (1u << rightSurround) | (1u << centreSurround)); }
AudioChannelSet AudioChannelSet::create6point0Music()  { return AudioChannelSet ((1u << left) | (1u << right) | (1u << leftSurround) | (1u << rightSurround) | (1u << leftSurroundSide) | (1u << rightSurroundSide)); }
AudioChannelSet AudioChannelSet::create6point1Music()  { return AudioChannelSet ((1u << left) | (1u << right) | (1u << LFE) | (1u << leftSurround) | (1u << rightSurround) | (1u << leftSurroundSide) | (1u << rightSurroundSide)); }
AudioChannelSet AudioChannelSet::create7point0()       { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurroundSide) | (1u << rightSurroundSide) | (1u << leftSurroundRear) | (1u << rightSurroundRear)); }
AudioChannelSet AudioChannelSet::create7point0SDDS()   { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurround) | (1u << rightSurround) | (1u << leftCentre) | (1u << rightCentre)); }
AudioChannelSet AudioChannelSet::create7point1()       { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << LFE) | (1u << leftSurroundSide) | (1u << rightSurroundSide) | (1u << leftSurroundRear) | (1u << rightSurroundRear)); }
AudioChannelSet AudioChannelSet::create7point1SDDS()   { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << LFE) | (1u << leftSurround) | (1u << rightSurround) | (1u << leftCentre) | (1u << rightCentre)); }
AudioChannelSet AudioChannelSet::quadraphonic()        { return AudioChannelSet ((1u << left) | (1u << right) | (1u << leftSurround) | (1u << rightSurround)); }
AudioChannelSet AudioChannelSet::pentagonal()          { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurroundRear) | (1u << rightSurroundRear)); }
AudioChannelSet AudioChannelSet::hexagonal()           { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << centreSurround) | (1u << leftSurroundRear) | (1u << rightSurroundRear)); }
AudioChannelSet AudioChannelSet::octagonal()           { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurround) | (1u << rightSurround) | (1u << centreSurround) | (1u << wideLeft) | (1u << wideRight)); }
AudioChannelSet AudioChannelSet::create7point0point2() { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << leftSurroundSide) | (1u << rightSurroundSide) | (1u << leftSurroundRear) | (1u << rightSurroundRear) | (1u << topSideLeft) | (1u << topSideRight)); }
AudioChannelSet AudioChannelSet::create7point1point2() { return AudioChannelSet ((1u << left) | (1u << right) | (1u << centre) | (1u << LFE) | (1u << leftSurroundSide) | (1u << rightSurroundSide) | (1u << leftSurroundRear) | (1u << rightSurroundRear) | (1u << topSideLeft) | (1u << topSideRight)); }

AudioChannelSet AudioChannelSet::discreteChannels (int numChannels)
{
    AudioChannelSet s;
    setBitRange(s.channels, discreteChannel0, numChannels);
    return s;
}

AudioChannelSet AudioChannelSet::canonicalChannelSet (int numChannels)
{
    if (numChannels == 1)  return AudioChannelSet::mono();
    if (numChannels == 2)  return AudioChannelSet::stereo();
    if (numChannels == 3)  return AudioChannelSet::createLCR();
    if (numChannels == 4)  return AudioChannelSet::quadraphonic();
    if (numChannels == 5)  return AudioChannelSet::create5point0();
    if (numChannels == 6)  return AudioChannelSet::create5point1();
    if (numChannels == 7)  return AudioChannelSet::create7point0();
    if (numChannels == 8)  return AudioChannelSet::create7point1();

    return discreteChannels (numChannels);
}

AudioChannelSet  AudioChannelSet::channelSetWithChannels (const QList<ChannelType>& channelArray)
{
    AudioChannelSet set;

    for (auto ch : channelArray)
    {
        Q_ASSERT (! set.channels[static_cast<int> (ch)]);

        set.addChannel (ch);
    }

    return set;
}

//==============================================================================
AudioChannelSet  AudioChannelSet::fromWaveChannelMask (quint32 dwChannelMask)
{
    return AudioChannelSet ((dwChannelMask & ((1 << 18) - 1)) << 1);
}

quint32 AudioChannelSet::getWaveChannelMask() const noexcept
{
    if (getHighestBit(channels) > topRearRight)
        return -1;

    std::bitset<32> result;
    for (int i=0; i<32; ++i) result.set(i, channels.test(i));

    return (result.to_ulong() >> 1);
}

} // namespace juce
