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

#pragma once

#include "sd_MeterHelpers.h"
#include "sd_MeterLevel.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sd
{
namespace SoundMeter
{
/**
 * @brief An individual meter channel.
 *
 * This represents a single meter.
 * Use the MetersComponent to create multiple meters matching
 * the specified channel format.
*/
class MeterChannel final
  : public juce::Component
  , private juce::SettableTooltipClient
{
public:
    using ChannelType = juce::AudioChannelSet::ChannelType;
    using Ptr         = juce::Component::SafePointer<MeterChannel>;

    /**
     * @brief Default constructor.
    */
    MeterChannel() noexcept;

    /**
     * @brief Parameterized constructor.
     *
     * @param meterOptions  Meter options to use (defining appearance and functionality).
     * @param padding       The padding to use (space between meter and the edge of the component).
     * @param channelName   The channel name (set by the user).
     * @param isLabelStrip  When set to true, this meter will function as a label strip (with markers for levels at the tick-marks).
     * @param channelType   The channel type (left, right, center, etc...).
    */
    MeterChannel (const Options& meterOptions, Padding padding, const juce::String& channelName, bool isLabelStrip = false, ChannelType channelType = ChannelType::unknown);

    /**
     * @brief Reset the meter (but not the peak hold).
     *
     * @see resetPeakHold
    */
    void reset();

    /**
     * @brief Refresh the meter with the current input level.
     *
     * @param forceRefresh When set to true, the meter will be forced to repaint (even if not dirty).
     * @see setRefreshRate
    */
    void refresh (bool forceRefresh);

    /**
     * @brief Sets the meter's refresh rate.
     *
     * Set this to optimize the meter's decay rate.
     *
     * @param refreshRate_hz Refresh rate in Hz.
     * @see refresh, setDecay, getDecay
    */
    void setRefreshRate (float refreshRate_hz) { m_level.setRefreshRate (refreshRate_hz); }

    /**
     * @brief Set meter decay.
     *
     * @param decay_ms Meter decay in milliseconds.
     * @see getDecay, setRefreshRate
    */
    void setDecay (float decay_ms) { m_level.setDecay (decay_ms); }

    /**
     * @brief Set the padding of the meter.
     *
     * The padding is the space between the meter and the edges
     * of the component.
     *
     * @param padding Amount of padding to apply.
    */
    void setPadding (const Padding& padding) noexcept { m_padding = padding; }

    /**
     * @brief Get meter decay.
     *
     * @return Meter decay in milliseconds.
     * @see setDecay, setRefreshRate
    */
    [[nodiscard]] float getDecay() const noexcept { return m_level.getDecay(); }

    /**
     * @brief Set the input level from the audio engine.
     *
     * Called from the audio thread!
     *
     * @param inputLevel New input level (in amp).
    */
    inline void setInputLevel (float inputLevel) { m_level.setInputLevel (inputLevel); }

    /**
     * @brief Set the meter's options.
     *
     * The options determine the appearance and functionality of the meter.
     *
     * @param meterOptions Meter options to use.
    */
    void setOptions (const Options& meterOptions);

    /**
     * @brief Activate or deactivate (mute) the meter.
     *
     * @param isActive When set to true, the meter is active.
     * @param notify   Determine whether to notify all listeners or not.
     * @see isActive
    */
    void setActive (bool isActive, NotificationOptions notify = NotificationOptions::dontNotify);

    /**
     * @brief Check if the meter is active (un-muted).
     *
     * @return True, if the meter is active (un-muted).
     *
     * @see setActive
    */
    [[nodiscard]] bool isActive() const noexcept { return m_active; }

    /**
     * @brief Set whether this meter is a label strip.
     *
     * A label strip only draws the value labels (at the tick-marks),
     * but does not display any level.
     *
     * @param isLabelStrip when set, this meter behave like a label strip.
    */
    void setIsLabelStrip (bool isLabelStrip = false) noexcept;

    /**
     * @brief Set the segments the meter is made out of.
     *
     * All segments have a level range, a range within the meter and a colour (or gradient).
     *
     * @param segmentsOptions The segments options to create the segments with.
    */
    void setMeterSegments (const std::vector<SegmentOptions>& segmentsOptions);

    /**
     * @brief Reset the peak hold.
     *
     * Resets the peak hold indicator and value.
     *
     * @see showPeakHold
    */
    void resetPeakHold();

    /**
     * @brief Show (or hide) the peak hold indicator.
     *
     * @param showPeakHold When set true, the peak hold indicator will be shown.
     * @see showPeakValue, resetPeakHold
    */
    void resetClipInd();

    /**
     * @brief Set the channel name.
     *
     * Set's the channel name belonging to the track
     * feeding the meter.
     *
     * @param channelName Name to assign to this meter.
    */
    void setChannelName (const juce::String& channelName);

    /**
     * @brief Get the width (in pixels) of the channel info in the 'header' part.
     *
     * @return The width (in pixels) taken by the channel info in the 'header' part.
     *
     * @see getChannelTypeWidth, nameFits, setChannelName
    */

    /** @internal */
    void paint (juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Colour IDs that can be used to customise the colours.
     * This can be done by overriding juce's LookAndFeel class.
    */
    enum ColourIds
    {
        backgroundColourId     = 0x1a03201,  ///< Background colour.
        tickMarkColourId       = 0x1a03202,  ///< Tick-mark colour.
        textColourId           = 0x1a03203,  ///< Text colour.
        textValueColourId      = 0x1a03205,  ///< Value text colour.
        inactiveColourId       = 0x1a03209,  ///< Inactive (muted) colour.
        peakHoldColourId       = 0x1a03210   ///< Peak hold colour.
    };

private:
    // clang-format off
    Level                       m_level             {};             ///< 'Meter' part of the meter. Actually displaying the level.
    Options                     m_meterOptions      {};             ///< 'Meter' options.

    bool                        m_active            = true;
    bool                        m_isLabelStrip      = false;

    juce::Rectangle<int>        m_dirtyRect         {};    
    Padding                     m_padding           { 0, 0, 0, 0 }; ///< Space between meter and component's edge.
    juce::FontOptions           m_font;
    MeterColours                m_meterColours      {};

    void                        setDirty            (bool isDirty = true) noexcept;
    [[nodiscard]] bool          isDirty             (const juce::Rectangle<int>& rectToCheck = {}) const noexcept;
    void                        addDirty            (const juce::Rectangle<int>& dirtyRect) noexcept;
    void                        drawMeter           (juce::Graphics& g);
    void                        mouseMove           (const juce::MouseEvent& event) override;
    void                        mouseExit           (const juce::MouseEvent& event) override;
    void                        mouseDoubleClick    (const juce::MouseEvent& event) override;
    void                        mouseDown           (const juce::MouseEvent& event) override;
    void                        resetMouseOvers     () noexcept;

    // clang-format on
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterChannel)
};
}  // namespace SoundMeter
}  // namespace sd
