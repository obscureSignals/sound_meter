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

#include "sd_MeterChannel.h"

namespace sd
{
namespace SoundMeter
{
MeterChannel::MeterChannel() noexcept
{
}
//==============================================================================

MeterChannel::MeterChannel (const Options& meterOptions, Padding padding, const juce::String& channelName, bool isLabelStrip /*= false*/,
                            ChannelType channelType /*= ChannelType::unknown*/)
  : MeterChannel()
{
    setName (channelName);
    setBufferedToImage (true);

    setOptions (meterOptions);
    setIsLabelStrip (isLabelStrip);
}
//==============================================================================

void MeterChannel::reset()
{
    m_level.reset();
    setDirty();
}

//==============================================================================

void MeterChannel::setOptions (const Options& meterOptions)
{
    m_meterOptions = meterOptions;

    setVisible (meterOptions.enabled);
    setEnabled (meterOptions.enabled);

    m_level.setMeterOptions (meterOptions);

    refresh (true);
}

//==============================================================================

void MeterChannel::resized()
{
    auto meterBounds = getLocalBounds();
    m_level.setMeterBounds (meterBounds);

}
//==============================================================================

void MeterChannel::paint (juce::Graphics& g)
{
    if (getLocalBounds().isEmpty())
        return;

    drawMeter (g);
}
//==============================================================================

void MeterChannel::drawMeter (juce::Graphics& g)
{
    // Draw meter BACKGROUND...

    g.setColour (m_active ? m_meterColours.backgroundColour : m_meterColours.inactiveColour);
    
    m_level.drawMeter (g, m_meterColours);
}
//==============================================================================

bool MeterChannel::isDirty (const juce::Rectangle<int>& rectToCheck /*= {}*/) const noexcept
{
    if (rectToCheck.isEmpty())
        return !m_dirtyRect.isEmpty();
    return m_dirtyRect.intersects (rectToCheck);
}
//==============================================================================

void MeterChannel::addDirty (const juce::Rectangle<int>& dirtyRect) noexcept
{
    if (!isShowing())
        return;
    m_dirtyRect = m_dirtyRect.getUnion (dirtyRect);
}
//==============================================================================

void MeterChannel::setDirty (bool isDirty /*= true*/) noexcept
{
    if (!isShowing())
        return;
    m_dirtyRect = { 0, 0, 0, 0 };
    if (isDirty)
        m_dirtyRect = getLocalBounds();
}
//==============================================================================

void MeterChannel::refresh (const bool forceRefresh)
{
    if (!isShowing())
        return;

    if (getBounds().isEmpty())
        return;

    if (m_active)
    {
        m_level.refreshMeterLevel();
        const auto levelDirtyBounds = m_level.getDirtyBounds();
        if (!levelDirtyBounds.isEmpty())
            addDirty (levelDirtyBounds);
    }

    // Redraw if dirty or forced to...
    if (isDirty())
    {
        repaint (m_dirtyRect);
        setDirty (false);
    }
    else if (forceRefresh)
    {
        repaint();
    }
}
//==============================================================================

void MeterChannel::setActive (bool isActive, NotificationOptions notify /*= NotificationOptions::dontNotify*/)
{
    if (m_active == isActive)
        return;
    m_active = isActive;

    juce::ignoreUnused (notify);

    reset();
    repaint();
}
//==============================================================================

void MeterChannel::resetMouseOvers() noexcept
{
    m_level.resetMouseOverValue();
    m_level.resetMouseOverClipInd();
    
}
//==============================================================================

void MeterChannel::resetPeakHold()
{
    m_level.resetPeakHold();
    // setDirty(); // TODO: - [mh] Change this to refresh(true)?
}
//==============================================================================

void MeterChannel::resetClipInd()
{
    m_level.resetClipInd();
}
//==============================================================================

void MeterChannel::setIsLabelStrip (bool isLabelStrip) noexcept
{
    m_isLabelStrip = isLabelStrip;
    m_level.setIsLabelStrip (isLabelStrip);
}
//==============================================================================

void MeterChannel::setMeterSegments (const std::vector<SegmentOptions>& segmentsOptions)
{
    m_level.setMeterSegments (segmentsOptions);
    setDirty();  // TODO: - [mh] Change this to refresh(true)?
}
//==============================================================================

void MeterChannel::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
    {
        if (m_level.isMouseOverClipInd (event.y))  // Clicking on clip indicator resets clip indicator ...
            resetClipInd();
    }
}

//==============================================================================

void MeterChannel::mouseMove (const juce::MouseEvent& event)
{
    // Check if the mouse is over the value part...
    const auto isMouseOverClipInd      = m_level.isMouseOverClipInd();
    const bool mouseOverClipIndChanged = (isMouseOverClipInd != m_level.isMouseOverClipInd (event.y));
    if (m_level.isMouseOverClipInd() && mouseOverClipIndChanged)
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);  // NOLINT
        setTooltip ("Click to clear peak hold."); // this doesnt work?
    }
    if (mouseOverClipIndChanged)
        addDirty (m_level.getClipIndBounds());
    
    // Check if the mouse is over the meter part...
    if (!m_level.isMouseOverClipInd())
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }
}
//==============================================================================

void MeterChannel::mouseExit (const juce::MouseEvent& /*event*/)
{
    resetMouseOvers();
    repaint();
}

//==============================================================================
void MeterChannel::mouseDoubleClick (const juce::MouseEvent& event)
{
}

}  // namespace SoundMeter
}  // namespace sd
