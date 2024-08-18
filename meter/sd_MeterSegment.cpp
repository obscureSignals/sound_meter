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

#include "sd_MeterSegment.h"

namespace sd  // NOLINT
{
namespace SoundMeter
{
Segment::Segment (const Options& meterOptions, const SegmentOptions& segmentOptions)
{
    setSegmentOptions (segmentOptions);
    setMeterOptions (meterOptions);
}

//==============================================================================

void Segment::setSegmentOptions (SegmentOptions segmentOptions)
{
    // Check level range validity.
    jassert (segmentOptions.levelRange.getLength() > 0.0f);  // NOLINT
    // Check meter range validity (0.0f - 1.0f).
    jassert (segmentOptions.meterRange.getStart() >= 0.0f && segmentOptions.meterRange.getEnd() <= 1.0f && segmentOptions.meterRange.getLength() > 0.0f);  // NOLINT

    m_segmentOptions = segmentOptions;

    if (!m_meterBounds.isEmpty())
        setMeterBounds (m_meterBounds);

    m_isDirty = true;
}

//==============================================================================

void Segment::draw (juce::Graphics& g, const MeterColours& meterColours)
{
    m_isDirty = false;

    if (m_isLabelStrip)
    {
        drawLabels (g, meterColours);
        return;
    }

    if (!m_drawnBounds.isEmpty())
    {
        g.setGradientFill (m_gradientFill);
        g.setOpacity(0.8);
        g.fillRect (m_drawnBounds);
    }

    if (m_meterOptions.showPeakHoldIndicator && !m_peakHoldBounds.isEmpty())
    {
        g.setGradientFill (m_gradientFill);
        g.setOpacity(0.8);
        g.fillRect (m_peakHoldBounds);
        m_drawnPeakHoldBounds = m_peakHoldBounds;
    }
}

//==============================================================================

void Segment::drawLabels (juce::Graphics& g, const MeterColours& meterColours) const
{
    g.setColour (juce::Colours::lightgrey);
    
    const float fontsize = 12;
    g.setFont (fontsize);
    
    for (const auto& tickMark: m_tickMarks)
    {
        const auto tickMarkLevelRatio = std::clamp ((tickMark - m_segmentOptions.levelRange.getStart()) / m_segmentOptions.levelRange.getLength(), 0.0f, 1.0f);
        
        const auto tickMarkY = m_segmentBounds.getY() + m_segmentBounds.proportionOfHeight (1.0f - tickMarkLevelRatio);
        
        const auto tickLabelString = juce::String (std::abs (tickMark));
        
        const auto strWidth = g.getCurrentFont().getStringWidth(tickLabelString);
        
        int tickWidth = floor((m_meterBounds.getWidth() - strWidth)/2)-2;
        
        const auto leftTickMarkBounds = juce::Rectangle<float> (m_meterBounds.getX(),
                                                                tickMarkY,
                                                                tickWidth,
                                                                static_cast<float> (Constants::kTickMarkHeight));
        g.fillRect (leftTickMarkBounds);
        const auto rightTickMarkBounds = juce::Rectangle<float> (m_meterBounds.getX()+m_meterBounds.getWidth()-tickWidth, 
                                                                 tickMarkY,
                                                                 tickWidth,
                                                                 static_cast<float> (Constants::kTickMarkHeight));
        g.fillRect (rightTickMarkBounds);
        
        const auto labelBounds        = juce::Rectangle<float> (0, tickMarkY - (fontsize / 2.0f), m_meterBounds.getWidth(), fontsize);
                    
        g.drawFittedText (tickLabelString, labelBounds.reduced (Constants::kLabelStripTextPadding, 0).toNearestInt(), juce::Justification::centred, 1);
    }
}
//==============================================================================

void Segment::setMeterBounds (juce::Rectangle<int> meterBounds)
{
    m_meterBounds            = meterBounds;
    auto       floatBounds   = meterBounds.toFloat();
    const auto segmentBounds = floatBounds.withY (floatBounds.getY() + floatBounds.proportionOfHeight (1.0f - m_segmentOptions.meterRange.getEnd()))
                                 .withHeight (floatBounds.proportionOfHeight (m_segmentOptions.meterRange.getLength()));
    m_segmentBounds = segmentBounds;
    updateLevelBounds();
    updatePeakHoldBounds();

    m_gradientFill = juce::ColourGradient (m_segmentOptions.segmentColour, segmentBounds.getBottomLeft(), m_segmentOptions.nextSegmentColour, segmentBounds.getTopLeft(), false);

    m_isDirty = true;
}
//==============================================================================

void Segment::setLevel (float level_db)
{
    if (level_db != m_currentLevel_db)
    {
        m_currentLevel_db = level_db;
        updateLevelBounds();
    }

    if (level_db > m_peakHoldLevel_db)
    {
        m_peakHoldLevel_db = level_db;
        updatePeakHoldBounds();
    }
}
//==============================================================================

void Segment::updateLevelBounds()
{
    if (m_segmentBounds.isEmpty())
        return;

    const auto levelRatio  = std::clamp ((m_currentLevel_db - m_segmentOptions.levelRange.getStart()) / m_segmentOptions.levelRange.getLength(), 0.0f, 1.0f);
    const auto levelBounds = m_segmentBounds.withTop (m_segmentBounds.getY() + m_segmentBounds.proportionOfHeight (1.0f - levelRatio));

    if (m_drawnBounds == levelBounds)
        return;

    m_drawnBounds = levelBounds;
    m_isDirty     = true;
}
//==============================================================================

void Segment::updatePeakHoldBounds()
{
    auto peakHoldBounds = juce::Rectangle<float>();

    if (Helpers::containsUpTo (m_segmentOptions.levelRange, m_peakHoldLevel_db))
    {
        const auto peakHoldRatio = std::clamp ((m_peakHoldLevel_db - m_segmentOptions.levelRange.getStart()) / m_segmentOptions.levelRange.getLength(), 0.0f, 1.0f);
        if (peakHoldRatio == 0.0f)
            return;

        const auto peakHoldY = m_segmentBounds.getY() + m_segmentBounds.proportionOfHeight (1.0f - peakHoldRatio);
        peakHoldBounds       = m_segmentBounds.withTop (peakHoldY).withHeight (Constants::kPeakHoldHeight);
    }

    if (peakHoldBounds == m_drawnPeakHoldBounds)
        return;

    m_peakHoldBounds = peakHoldBounds;
    m_isDirty        = true;
}
//==============================================================================

void Segment::resetPeakHold() noexcept
{
    m_peakHoldBounds.setHeight (0);
    m_peakHoldLevel_db    = Constants::kMinLevel_db;
    m_drawnPeakHoldBounds = m_peakHoldBounds;
    m_isDirty             = true;
}
//==============================================================================

void Segment::setMeterOptions (const Options& meterOptions)
{
    m_meterOptions = meterOptions;

    // Find all tickMark-marks in this segment's range...
    m_tickMarks.clear();
    for (const auto& tickMark: meterOptions.tickMarks)
    {
        if (Helpers::containsUpTo (m_segmentOptions.levelRange, tickMark))
            m_tickMarks.emplace_back (tickMark);
    }

    m_isDirty = true;
}
//==============================================================================
}  // namespace SoundMeter
}  // namespace sd
