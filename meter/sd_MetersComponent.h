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

#include "sd_MeterChannel.h"
#include "sd_MeterHelpers.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * @brief Namespace containing all concerning the sound_meter module.
*/
namespace sd  // NOLINT
{
namespace SoundMeter
{
class MeterChannel;

/**
 * @brief Component containing one or more meters.
 *
 * After setting the channel format it
 * will automatically create the needed meters and give them proper names.
 */
class MetersComponent final
  : public juce::Component
  , private juce::Timer
{
public:
    /**
     * @brief Default constructor.
    */
    MetersComponent();

    /** @brief Destructor.*/
    ~MetersComponent() override;

    /**
     * @brief Refresh (redraw) the meters panel.
     *
     * This can be called manually or internally (see useInternalTiming).
     *
     * @param forceRefresh When set to true, always redraw the meters panel (not only if it's dirty/changed).
     *
     * @see setRefreshRate, useInternalTiming
    */
    void refresh (bool forceRefresh = false);

    /**
     * @brief Reset the meters.
     *
     * Initialise the meters, faders and clears all the levels (but not preserves the peak hold).
     *
     * @see resetPeakHold, resetMeters
    */
    void reset();

    /**
     * @brief Reset all meters.
     *
     * Resets all meters to 0 (but not the peak hold).
     * @see reset, resetPeakHold
    */
    void resetMeters();

    /**
     * @brief Clear the level of the meters.
    */
    void clearMeters();

    /**
     * @brief Reset all peak hold indicators and 'values'.
     *
     * @see reset, resetMeters
    */
    void resetPeakHold();

    /**
     * @brief Set the input level.
     *
     * This supplies a meter of a specific channel with the peak level from the audio engine.
     * Beware: this will usually be called from the audio thread.
     *
     * @param channel The channel to set the input level of.
     * @param value   The input level to set to the specified channel.
    */
    void setInputLevel (int channel, float value);

    /**
     * @brief Set meter options defining appearance and functionality.
     *
     * @param meterOptions The options to apply to the meters and label strip.
    */
    void setOptions (const Options& meterOptions);

    /**
     * @brief Set the refresh (redraw) rate of the meters.
     *
     * Also used for meter ballistics.
     * When using the internal timer (setInternalTiming) this set's it's refresh rate.
     * When manually redrawing (with refresh) you could (should) still provide the refresh rate
     * to optimize a smooth decay.
     *
     * @param refreshRate The refresh rate (in Hz).
     *
     * @see setDecay, refresh, setInternalTiming
    */
    void setRefreshRate (float refreshRate);

    /**
     * @brief Set the segments the meter is made out of.
     *
     * All segments have a level range, a range within the meter and a colour (or gradient).
     *
     * @param segmentsOptions The segments options to create the segments with.
    */
    void setMeterSegments (const std::vector<SegmentOptions>& segmentsOptions);

    /**
     * @brief Enable or disable the panel.
     *
     * @param enabled When set to true, the meters panel will be displayed.
    */
    void enable (bool enabled = true);


    /** @internal */
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // clang-format off

   Options                          m_meterOptions          {};  
   std::vector<SegmentOptions>      m_segmentsOptions       = MeterScales::getDefaultScale();

   using                            MetersType              = juce::OwnedArray<MeterChannel>;
   MetersType                       m_meterChannels         {};
   MeterChannel                     m_labelStrip            {};

   bool                             m_useInternalTimer      = true;
   juce::FontOptions                m_font;


   // Private methods...
   void                             timerCallback           () override { refresh(); }
//   void                             setColours              ();
   void                             createMeters            (const juce::AudioChannelSet& channelFormat, const std::vector<juce::String>& channelNames);
   void                             deleteMeters            ();
   [[nodiscard]] MeterChannel*      getMeterChannel         (int meterIndex) noexcept;     


    // clang-format on
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MetersComponent)
};
}  // namespace SoundMeter
}  // namespace sd
