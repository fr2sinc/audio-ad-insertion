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
class AdInsertionAudioProcessorEditor : public juce::AudioProcessorEditor,
	public Slider::Listener,
	private juce::Timer
{
public:
	AdInsertionAudioProcessorEditor(AdInsertionAudioProcessor&);
	~AdInsertionAudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	AdInsertionAudioProcessor& audioProcessor;

	Slider mDelaySlider;
	void sliderValueChanged(Slider* slider) override;

	void timerCallback() override;

	juce::Label curJingle1Label;
	Label boxLabelJ1;

	juce::Label curJingle2Label;
	Label boxLabelJ2;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdInsertionAudioProcessorEditor)
};
