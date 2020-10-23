/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdInsertionAudioProcessorEditor::AdInsertionAudioProcessorEditor(AdInsertionAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	startTimerHz(60);
	/*mDelaySlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	mDelaySlider.setTextBoxStyle(Slider::TextBoxBelow, true, 50, 20);
	mDelaySlider.setRange(0.0f, 2000.0f, 10.0f);
	mDelaySlider.setValue(0.0f);
	mDelaySlider.addListener(this);
	addAndMakeVisible(&mDelaySlider);*/

	addAndMakeVisible(curJingle1Label);
	curJingle1Label.setColour(juce::Label::backgroundColourId, juce::Colours::yellow);
	curJingle1Label.setColour(juce::Label::textColourId, juce::Colours::black);
	curJingle1Label.setText("", juce::dontSendNotification);
	curJingle1Label.setJustificationType(juce::Justification::centred);
	
	boxLabelJ1.setText("Riconoscimento del Jingle di apertura:", dontSendNotification);
	boxLabelJ1.attachToComponent(&curJingle1Label, false);

	addAndMakeVisible(curJingle2Label);
	curJingle2Label.setColour(juce::Label::backgroundColourId, juce::Colours::yellow);
	curJingle2Label.setColour(juce::Label::textColourId, juce::Colours::black);
	curJingle2Label.setText("", juce::dontSendNotification);
	curJingle2Label.setJustificationType(juce::Justification::centred);

	boxLabelJ2.setText("Riconoscimento del Jingle di chiusura:", dontSendNotification);
	boxLabelJ2.attachToComponent(&curJingle2Label, false);

	setSize(500, 300);
}

AdInsertionAudioProcessorEditor::~AdInsertionAudioProcessorEditor()
{
}

//==============================================================================
void AdInsertionAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(Colours::black);
}

void AdInsertionAudioProcessorEditor::resized()
{
	//mDelaySlider.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 75, 100, 150);
	curJingle1Label.setBounds(10, 80, getWidth() - 20, 40);
	curJingle2Label.setBounds(10, 180, getWidth() - 20, 40);

}

void AdInsertionAudioProcessorEditor::sliderValueChanged(Slider * slider)
{
	//check if the *slider points to a memory location
	if (slider == &mDelaySlider)
		audioProcessor.mDelay = mDelaySlider.getValue();
}

void AdInsertionAudioProcessorEditor::timerCallback() {
	if (audioProcessor.samplesRemainingDelayedJ1 == 0) {
		curJingle1Label.setText("", juce::dontSendNotification);
	}
	else {
		double timeRemaining = (double)audioProcessor.samplesRemainingDelayedJ1 / (double)audioProcessor.mSampleRate;
		std::ostringstream oss;
		oss << audioProcessor.curJingle1 << ": tempo rimanente: " << timeRemaining;
		curJingle1Label.setText(oss.str(), juce::dontSendNotification);
	}

	if (audioProcessor.samplesRemainingDelayedJ2 == 0) {
		curJingle2Label.setText("", juce::dontSendNotification);
	}
	else {
		double timeRemaining = (double)audioProcessor.samplesRemainingDelayedJ2 / (double)audioProcessor.mSampleRate;
		std::ostringstream oss;
		oss << audioProcessor.curJingle2 << ": tempo rimanente: " << timeRemaining;
		curJingle2Label.setText(oss.str(), juce::dontSendNotification);
	}
}
