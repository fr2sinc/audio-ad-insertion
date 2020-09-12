#pragma once

#include <JuceHeader.h>
#include "../../Source/FingerprintLive.h"
#include "../../Source/JsonUtility.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
	

private:
	void loadButtonClicked();

	void sampleRateMenuChanged();

	juce::TextButton loadButton;
	juce::ComboBox sampleRateMenu;

	double sampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
