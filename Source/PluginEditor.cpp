/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FFTimplAudioProcessorEditor::FFTimplAudioProcessorEditor(FFTimplAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	startTimerHz(60);

	mDelaySlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	mDelaySlider.setTextBoxStyle(Slider::TextBoxBelow, true, 50, 20);
	mDelaySlider.setRange(0.0f, 2000.0f, 10.0f);
	mDelaySlider.setValue(0.0f);
	mDelaySlider.addListener(this);

	addAndMakeVisible(&mDelaySlider);

	addAndMakeVisible(curJingleLabel);
	curJingleLabel.setColour(juce::Label::backgroundColourId, juce::Colours::yellow);
	curJingleLabel.setColour(juce::Label::textColourId, juce::Colours::black);
	curJingleLabel.setText("", juce::dontSendNotification);
	curJingleLabel.setJustificationType(juce::Justification::centred);

	setSize(600, 400);
}

FFTimplAudioProcessorEditor::~FFTimplAudioProcessorEditor()
{
}

//==============================================================================
void FFTimplAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(Colours::black);
}

void FFTimplAudioProcessorEditor::resized()
{
	mDelaySlider.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 75, 100, 150);
	curJingleLabel.setBounds(10, 80, getWidth() - 20, 40);
}

void FFTimplAudioProcessorEditor::sliderValueChanged(Slider * slider)
{
	//check if the *slider points to a memory location
	if (slider == &mDelaySlider)
		audioProcessor.mDelay = mDelaySlider.getValue();
}

void FFTimplAudioProcessorEditor::timerCallback() {
		curJingleLabel.setText(audioProcessor.curJingle, juce::dontSendNotification);
}
