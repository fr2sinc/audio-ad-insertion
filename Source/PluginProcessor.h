/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GoertzelAnalyzer.h"
#include "Fingerprint.h"
#include "FingerprintLive.h"
#include "JsonUtility.h"
//==============================================================================
/**
*/
class AdInsertionAudioProcessor : public juce::AudioProcessor
{
public:
	enum ToneState
	{
		On,
		Off
	};

	enum fPrintState
	{
		fSearchingJ1,
		fRecognizedJ1,
		fSearchingJ2,
		fRecognizedJ2
	};

	enum injectionState1
	{
		waitJ1End,
		notActiveInj1
	};

	enum injectionState2
	{
		waitAdEnd,
		waitJ2,
		notActiveInj2
	};

	//==============================================================================
	AdInsertionAudioProcessor();
	void initializeFprint();
	~AdInsertionAudioProcessor() override;

	//==============================================================================
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	void doFprintAnalysis(int channel, const int bufferLength, juce::AudioBuffer<float> buffer);

	void doToneAnalysis(int channel, const int bufferLength, const float* bufferData);

	void changeFprintState(const int bufferLength);

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
	juce::String curJingle1;
	juce::String curJingle2;
	int samplesRemainingDelayedJ1 = 0;
	int samplesRemainingDelayedJ2 = 0;

	int j1SamplesRemaining = 0;
	int j2SamplesRemaining = 0;

	int samplesAdRemaining = 0;
	int second_jingle_duration = 0;
	int mSampleRate{ 0 };

private:
	AudioBuffer<float> mDelayBuffer;
	int mWritePosition{ 0 };
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
	FingerprintLive fprintLive;
	fPrintState fState = fSearchingJ1;
	injectionState1 injState1 = notActiveInj1;
	injectionState2 injState2 = notActiveInj2;

	int delayInSamples;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdInsertionAudioProcessor)
};
