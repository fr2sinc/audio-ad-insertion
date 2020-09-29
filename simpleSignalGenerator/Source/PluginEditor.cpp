/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleSignalGeneratorAudioProcessorEditor::SimpleSignalGeneratorAudioProcessorEditor (SimpleSignalGeneratorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
	mute = true;

	//volume slider
	addAndMakeVisible(volumeSlider);
	volumeSlider.setRange(-96.0, 0.0);
	volumeSlider.setTextValueSuffix(" db");
	volumeSlider.setValue(-6.0);
	volumeSlider.addListener(this);	

	volumeLabel.setText("Volume", dontSendNotification);
	volumeLabel.attachToComponent(&volumeSlider, true);
	
	//freq slider
	addAndMakeVisible(freqSlider);
	freqSlider.setRange(10, 22000);
	freqSlider.setTextValueSuffix(" Hz");
	freqSlider.setValue(500.0);
	freqSlider.addListener(this);
	freqSlider.setSkewFactorFromMidPoint(500);

	freqLabel.setText("Freq", dontSendNotification);
	freqLabel.attachToComponent(&freqSlider, true);

	//phase slider
	addAndMakeVisible(phaseSlider);
	phaseSlider.setRange(0.0, 1.0);
	phaseSlider.setTextValueSuffix(" ~");
	phaseSlider.setValue(0.0);
	phaseSlider.addListener(this);

	phaseLabel.setText("Phase", dontSendNotification);
	phaseLabel.attachToComponent(&phaseSlider, true);

	//mute button
	addAndMakeVisible(muteButton);
	muteButton.setButtonText("On");	
	muteButton.addListener(this);
	muteButton.setEnabled(true);

	setSize(480, 140);
}

SimpleSignalGeneratorAudioProcessorEditor::~SimpleSignalGeneratorAudioProcessorEditor()
{
}

//==============================================================================
void SimpleSignalGeneratorAudioProcessorEditor::paint (juce::Graphics& g)
{
    //(Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);    
}


void SimpleSignalGeneratorAudioProcessorEditor::resized()
{
	const int sliderLeft = 50;
	volumeSlider.setBounds(sliderLeft, 20, getWidth() - sliderLeft - 10, 20);
	freqSlider.setBounds(sliderLeft, 50, getWidth() - sliderLeft - 10, 20);
	phaseSlider.setBounds(sliderLeft, 80, getWidth() - sliderLeft - 10, 20);	
	muteButton.setBounds(10, 110, getWidth() - 20, 20);
}

void SimpleSignalGeneratorAudioProcessorEditor::buttonClicked(Button* button) {
	if (button == &muteButton) {
		mute = !mute;
		if(mute)
			muteButton.setButtonText("On");
		else
			muteButton.setButtonText("Off");

		audioProcessor.mute = mute;
	}
}

void SimpleSignalGeneratorAudioProcessorEditor::sliderValueChanged(Slider *slider) {
	if (slider == &volumeSlider) {
		audioProcessor.amplitude = pow(10, ((float)volumeSlider.getValue() / 20.0));
	}

	if (slider == &freqSlider) {
		audioProcessor.frequency = (float)freqSlider.getValue();
	}

	if (slider == &phaseSlider) {
		audioProcessor.phase = (float)phaseSlider.getValue();
	}
}
