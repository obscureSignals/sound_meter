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

#include "sd_MeterLevel.h"

namespace sd  // NOLINT
{
namespace SoundMeter
{
Level::Level()
{
    setMeterSegments (m_segmentOptions);
}
//==============================================================================

void Level::drawMeter (juce::Graphics& g, const MeterColours& meterColours)
{
    const auto currentTime = static_cast<int> (juce::Time::getMillisecondCounter());
    const auto timePassed  = static_cast<float> (currentTime - static_cast<int> (m_previousPeakHoldTime));
    m_totalPeakHoldTimePassed = m_totalPeakHoldTimePassed + timePassed;
    m_previousPeakHoldTime = currentTime;
    
    if (m_totalPeakHoldTimePassed >= m_meterOptions.peakDecayTime_ms){
        m_totalPeakHoldTimePassed = 0.0f;
        resetPeakHold();
    }
    
    setClipInd ();
    
    for (auto& segment: m_segments)
        segment.draw (g, meterColours);
    
    if (!m_valueBounds.isEmpty())
        drawPeakValue (g, meterColours);
    
    if (!m_meterBounds.isEmpty() && !m_isLabelStrip)
        drawClipInd(g, meterColours);

}
//==============================================================================

void Level::drawPeakValue (juce::Graphics& g, const MeterColours& meterColours) const
{
    if (m_valueBounds.isEmpty())
        return;

    // Draw PEAK value...
    const auto peak_db = getPeakHoldLevel();
        
    if (peak_db > m_meterRange.getStart())  // If active, present and enough space is available.
    {
        const int precision = peak_db <= -10.0f ? 1 : 2;  // Set precision depending on peak value. NOLINT
        g.setColour (meterColours.textValueColour);
        g.drawFittedText (juce::String (peak_db, precision), m_valueBounds, juce::Justification::centred, 1);
    }
}
//==============================================================================

void Level::drawClipInd (juce::Graphics& g, const MeterColours& meterColours) const
{
    if (m_clip) {
        g.setFillType(juce::FillType(m_meterOptions.clipIndicatorColor));
        g.fillRect(m_clipIndBounds.getX(), m_clipIndBounds.getY(), m_clipIndBounds.getWidth(), m_clipIndBounds.getHeight());
    }
}
//==============================================================================

void Level::setClipInd ()
{
    const auto peak_db = getPeakHoldLevel();
    if (peak_db >= 0.f){
        m_clip = true;
        m_clipDirty = true;
    }
}
//==============================================================================\

void Level::resetClipInd ()
{
    m_clip = false;
    m_clipDirty = true;
}
//==============================================================================

float Level::getInputLevel()
{
    m_inputLevelRead.store (true);
    return m_meterRange.clipValue (juce::Decibels::gainToDecibels (m_inputLevel.load()));
}
//==============================================================================

void Level::setInputLevel (float newLevel)
{
    m_inputLevel.store (m_inputLevelRead.load() ? newLevel : std::max (m_inputLevel.load(), newLevel));
    m_inputLevelRead.store (false);
}
//==============================================================================

float Level::getLinearDecayedLevel (float newLevel_db)
{
    const auto currentTime = static_cast<int> (juce::Time::getMillisecondCounter());
    const auto timePassed  = static_cast<float> (currentTime - static_cast<int> (m_previousRefreshTime));

    m_previousRefreshTime = currentTime;
    
    if (newLevel_db >= m_meterLevel_db)
        return newLevel_db;

    return std::max (newLevel_db, m_meterLevel_db - (timePassed * m_decayRate));
}
//==============================================================================

void Level::refreshMeterLevel()
{
    setClipInd();
    
    m_meterLevel_db = getLinearDecayedLevel (getInputLevel());

    if (m_meterLevel_db > getPeakHoldLevel())
        m_peakHoldDirty = true;

    for (auto& segment: m_segments)
        segment.setLevel (m_meterLevel_db);
}
//==============================================================================

void Level::setMeterOptions (const Options& meterOptions)
{
    m_meterOptions = meterOptions;

    calculateDecayCoeff (meterOptions);
    synchronizeMeterOptions();
    
}
//==============================================================================

void Level::synchronizeMeterOptions()
{
    for (auto& segment: m_segments)
    {
        segment.setMeterOptions (m_meterOptions);
        segment.setIsLabelStrip (m_isLabelStrip);
    }

    m_peakHoldDirty = true;
}
//==============================================================================

void Level::setMeterSegments (const std::vector<SegmentOptions>& segmentsOptions)
{
    m_segments.clear();
    for (const auto& segmentOptions: segmentsOptions)
    {
        m_segments.emplace_back (m_meterOptions, segmentOptions);
        m_meterRange.setStart (std::min (m_meterRange.getStart(), segmentOptions.levelRange.getStart()));
        m_meterRange.setEnd (std::max (m_meterRange.getEnd(), segmentOptions.levelRange.getEnd()));
    }
    for (auto& segment: m_segments)
        segment.setMeterBounds (m_levelBounds);
    synchronizeMeterOptions();
    calculateDecayCoeff (m_meterOptions);
}
//==============================================================================

void Level::reset()
{
    m_inputLevel.store (0.0f);
    m_meterLevel_db       = Constants::kMinLevel_db;
    m_previousRefreshTime = 0;
}
//==============================================================================

void Level::setIsLabelStrip (bool isLabelStrip) noexcept
{
    m_isLabelStrip = isLabelStrip;
    synchronizeMeterOptions();
}
//==============================================================================

void Level::setRefreshRate (float refreshRate_hz)
{
    m_meterOptions.refreshRate = refreshRate_hz;
    calculateDecayCoeff (m_meterOptions);
    synchronizeMeterOptions();
}
//==============================================================================

void Level::setDecay (float decay_ms)
{
    m_meterOptions.decayTime_ms = decay_ms;
    calculateDecayCoeff (m_meterOptions);
    synchronizeMeterOptions();
}
//==============================================================================

void Level::resetPeakHold()
{
    for (auto& segment: m_segments)
        segment.resetPeakHold();
    m_peakHoldDirty = true;
}
//==============================================================================

float Level::getPeakHoldLevel() const noexcept
{
    if (m_segments.empty())
        return Constants::kMinLevel_db;

    return m_segments[0].getPeakHold();
    
}
//==============================================================================

void Level::setMeterBounds (const juce::Rectangle<int>& bounds)
{
    if (bounds == m_meterBounds)
        return;
    
    m_meterBounds = bounds;
    m_levelBounds = m_meterBounds;

    if (m_meterOptions.valueEnabled)
        m_valueBounds = m_levelBounds.removeFromBottom(Constants::kDefaultHeaderHeight);
    else
        m_valueBounds = juce::Rectangle<int>();
    
    if (m_meterOptions.showClipIndicator)
        m_clipIndBounds = m_levelBounds.removeFromTop (12);
    else
        m_clipIndBounds = juce::Rectangle<int>();
    
    for (auto& segment: m_segments)
        segment.setMeterBounds (m_levelBounds);
    
    if (m_meterOptions.showClipIndicator)
        m_clipIndBounds.setHeight(6);

    if (m_isLabelStrip)
        m_clipIndBounds = juce::Rectangle<int>();

    m_peakHoldDirty = true;
    m_clipDirty = true;
    
}
//==============================================================================

juce::Rectangle<int> Level::getDirtyBounds()
{
    juce::Rectangle<int> dirtyBounds {};
    for (const auto& segment: m_segments)
    {
        if (segment.isDirty())
            dirtyBounds = dirtyBounds.getUnion (segment.getSegmentBounds().toNearestIntEdges());
    }

    if (m_peakHoldDirty)
    {
        dirtyBounds     = dirtyBounds.getUnion (m_valueBounds);
        m_peakHoldDirty = false;
    }
    
    if (m_clipDirty)
    {
        dirtyBounds     = dirtyBounds.getUnion (m_clipIndBounds);
        m_clipDirty     = false;
    }

    return dirtyBounds;
}
//==============================================================================

void Level::calculateDecayCoeff (const Options& meterOptions)
{
    m_meterOptions.decayTime_ms = juce::jlimit (Constants::kMinDecay_ms, Constants::kMaxDecay_ms, meterOptions.decayTime_ms);
    m_meterOptions.refreshRate  = std::max (1.0f, meterOptions.refreshRate);
    m_refreshPeriod_ms          = (1.0f / m_meterOptions.refreshRate) * 1000.0f;  // NOLINT

    m_decayRate = m_meterRange.getLength() / m_meterOptions.decayTime_ms;
}
//==============================================================================

bool Level::isMouseOverClipInd (const int y)
{
    m_mouseOverClipInd = (y >= m_clipIndBounds.getY() && !m_clipIndBounds.isEmpty());
    return m_mouseOverClipInd;
}


}  // namespace SoundMeter
}  // namespace sd
