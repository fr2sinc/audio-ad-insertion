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
class SimpleSignalGeneratorAudioProcessorEditor  :	public juce::AudioProcessorEditor,
													public Slider::Listener,
													public TextButton::Listener
							
{
public:
    SimpleSignalGeneratorAudioProcessorEditor (SimpleSignalGeneratorAudioProcessor&);
    ~SimpleSignalGeneratorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

	void buttonClicked(Button * button) override;

	void sliderValueChanged(Slider * slider) override;

private:    
    SimpleSignalGeneratorAudioProcessor& audioProcessor;

	//user interface
	Slider volumeSlider;
	Slider freqSlider;
	Slider phaseSlider;
	Label volumeLabel;
	Label freqLabel;
	Label phaseLabel;
	TextButton muteButton;

	bool mute;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleSignalGeneratorAudioProcessorEditor)
};
