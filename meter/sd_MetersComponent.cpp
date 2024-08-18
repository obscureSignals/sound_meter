/*
    ==============================================================================

    This file is part of the sound_meter JUCE module
    Copyright (c) 2019 - 2021 Sound Development - Marcel Huibers
    All rights reserved.

    ------------------------------------------------------------------------------

    sound_meter is provided under the terms of The MIT License (MIT):

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    ==============================================================================
*/

#include "sd_MetersComponent.h"

namespace sd  // NOLINT
{
namespace SoundMeter
{
MetersComponent::MetersComponent()
    : m_meterOptions ({}),
    m_labelStrip ({}, Padding (0, 0, 0, 0), "label_strip", true, juce::AudioChannelSet::ChannelType::unknown)
{
    setName ("meters_panel");
    addAndMakeVisible (m_labelStrip);
    startTimerHz (static_cast<int> (std::round (m_meterOptions.refreshRate)));
    createMeters (juce::AudioChannelSet::stereo(), {});
}

//==============================================================================

MetersComponent::~MetersComponent()
{
    deleteMeters();
}
//==============================================================================

void MetersComponent::reset()
{
    deleteMeters();

    refresh (true);
}
//==============================================================================

void MetersComponent::clearMeters()
{
    for (auto* meter: m_meterChannels)
        if (meter)
            meter->setInputLevel (0.0f);

    refresh (true);
}
//==============================================================================

void MetersComponent::refresh (const bool forceRefresh /*= false*/)
{
    if (!isShowing() || m_meterChannels.isEmpty())
        return;

    
    for (auto* meter: m_meterChannels)
    {
        if (meter)
            meter->refresh (forceRefresh);
    }
    
    m_labelStrip.refresh (forceRefresh);

}
//==============================================================================

void MetersComponent::setRefreshRate (float refreshRate_hz)
{
    m_meterOptions.refreshRate = refreshRate_hz;

    m_labelStrip.setRefreshRate (static_cast<float> (refreshRate_hz));
    for (auto* meter: m_meterChannels)
        if (meter)
            meter->setRefreshRate (static_cast<float> (refreshRate_hz));

    if (m_useInternalTimer)
    {
        stopTimer();
        startTimerHz (juce::roundToInt (refreshRate_hz));
    }
}
//==============================================================================

void MetersComponent::paint (juce::Graphics& g)
{
}
//==============================================================================

void MetersComponent::resized()
{
    auto        panelBounds     = getLocalBounds().toFloat();
    
    m_labelStrip.setBounds(panelBounds.getX(), panelBounds.getY(), panelBounds.getWidth(), panelBounds.getHeight());

    const int   panelWidth      = panelBounds.getWidth();
    int         meterWidth;
    int         gap;
    if (panelWidth % 2 == 0){
        meterWidth = (panelWidth/2)-1;
        gap = 2;
    }
    else {
        meterWidth = (panelWidth-1)/2;
        gap = 1;
    }
    
    m_meterChannels[0]->setBounds (panelBounds.removeFromLeft(meterWidth).toNearestIntEdges());
    panelBounds.removeFromLeft(gap);
    m_meterChannels[1]->setBounds (panelBounds.removeFromLeft(meterWidth).toNearestIntEdges());
}

//==============================================================================

void MetersComponent::setInputLevel (int channel, float value)
{
    if (auto* meterChannel = getMeterChannel (channel))
        if (meterChannel)
            meterChannel->setInputLevel (value);
}
//==============================================================================

void MetersComponent::createMeters (const juce::AudioChannelSet& channelFormat, const std::vector<juce::String>& channelNames)
{
    // Create enough meters to match the channel format...
    for (int channelIdx = 0; channelIdx < channelFormat.size(); ++channelIdx)
    {
        auto meterChannel = std::make_unique<MeterChannel> (m_meterOptions, Padding (0, 0, 0, 0), "meters_panel", false,
                                                            channelFormat.getTypeOfChannel (channelIdx));

        meterChannel->addMouseListener (this, true);

        addChildComponent (meterChannel.get());
        m_meterChannels.add (meterChannel.release());

        m_labelStrip.setActive (true);
        
    }
    setMeterSegments (m_segmentsOptions);
}
//==============================================================================

void MetersComponent::deleteMeters()
{
    m_meterChannels.clear();
}
//==============================================================================

MeterChannel* MetersComponent::getMeterChannel (const int meterIndex) noexcept
{
    return (juce::isPositiveAndBelow (meterIndex, m_meterChannels.size()) ? m_meterChannels[meterIndex] : nullptr);
}
//==============================================================================

void MetersComponent::resetMeters()
{
    for (auto* meter: m_meterChannels)
        if (meter)
            meter->reset();
}
//==============================================================================

void MetersComponent::resetPeakHold()
{
    for (auto* meter: m_meterChannels)
        if (meter)
            meter->resetPeakHold();
}
//==============================================================================

void MetersComponent::setOptions (const Options& meterOptions)
{
    m_meterOptions = meterOptions;
    for (auto* meter: m_meterChannels)
    {
        if (meter)
            meter->setOptions (meterOptions);
    }
    m_labelStrip.setOptions (meterOptions);

    setRefreshRate (meterOptions.refreshRate);
}
//==============================================================================

void MetersComponent::enable (bool enabled /*= true*/)
{
    m_meterOptions.enabled = enabled;

    for (auto* meter: m_meterChannels)
    {
        if (meter)
        {
            meter->setEnabled (enabled);
            meter->setVisible (enabled);
        }
    }

    m_labelStrip.setEnabled (enabled);
    m_labelStrip.setVisible (enabled);

    refresh (true);
}

void MetersComponent::setMeterSegments (const std::vector<SegmentOptions>& segmentsOptions)
{
    m_segmentsOptions = segmentsOptions;
    for (auto* meter: m_meterChannels)
        if (meter)
            meter->setMeterSegments (m_segmentsOptions);
}
//==============================================================================

}  // namespace SoundMeter
}  // namespace sd
