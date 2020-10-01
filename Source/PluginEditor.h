/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class FFTimplAudioProcessorEditor : public juce::AudioProcessorEditor,
	public Slider::Listener,
	private juce::Timer
{
public:
	FFTimplAudioProcessorEditor(FFTimplAudioProcessor&);
	~FFTimplAudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	FFTimplAudioProcessor& audioProcessor;

	Slider mDelaySlider;
	void sliderValueChanged(Slider* slider) override;

	void timerCallback() override;

	juce::Label curJingleLabel;
	Label boxLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTimplAudioProcessorEditor)
};
