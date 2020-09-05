/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GoertzelAnalyzer.h"
#include "Fingerprint.h"

//==============================================================================
/**
*/
class FFTimplAudioProcessor : public juce::AudioProcessor
{
public:
	enum ToneState
	{
		On,
		Off
	};

	//==============================================================================
	FFTimplAudioProcessor();
	~FFTimplAudioProcessor() override;

	//==============================================================================
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	void doToneAnalysis(int channel, const int bufferLength, const float* bufferData);

	void changeToneState();

	void fillDelayBuffer(int channel, const int bufferLength, const int delayBufferLength,
		const float* bufferData, const float* delayBufferData);

	void getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, const int bufferLength,
		const int delayBufferLength, const float* bufferData, const float* delayBufferData);

	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	//==============================================================================
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	
	float mDelay{ 0.0 };	

private:
	AudioBuffer<float> mDelayBuffer;
	int mWritePosition{ 0 };
	int mSampleRate{ 0 };
	bool toneOn = false;
	bool audioInjection = false;
	int timeCounter = 0;

	//needed for reading audio files from a new source
	juce::AudioFormatManager formatManager;
	std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
	juce::AudioTransportSource transportSource;
	ToneState toneState = Off;
	ToneState newToneState = Off;

	GoertzelAnalyzer gAnalyzer;
	Fingerprint fprint;
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTimplAudioProcessor)
};
