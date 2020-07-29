/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FFTimplAudioProcessor::FFTimplAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
}

FFTimplAudioProcessor::~FFTimplAudioProcessor()
{
}

//==============================================================================
const juce::String FFTimplAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool FFTimplAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool FFTimplAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool FFTimplAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double FFTimplAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int FFTimplAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int FFTimplAudioProcessor::getCurrentProgram()
{
	return 0;
}

void FFTimplAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String FFTimplAudioProcessor::getProgramName(int index)
{
	return {};
}

void FFTimplAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void FFTimplAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback

	const int numInputChannels = getTotalNumInputChannels();
	//set the mSampleRate variable to be used in all functions
	mSampleRate = sampleRate;

	//now I need the num of samples to fit in the delay buffer
	const int delayBufferSize = 2 * (sampleRate + samplesPerBlock); //2sec of delay buffer
	mDelayBuffer.setSize(numInputChannels, delayBufferSize);

	fft.setSampleRate(sampleRate);
}

void FFTimplAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FFTimplAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void FFTimplAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	const int bufferLength = buffer.getNumSamples();
	const int delayBufferLength = mDelayBuffer.getNumSamples();

	for (int channel = 0; channel < totalNumInputChannels; ++channel) {

		//--------------------------------
		//calculate FFT only in one channel, if i need to calculate fft on two channel I have to create e mono channel from two channel in a trivial way
		if (channel == 0) {
			//DBG(fft.getFundamentalFrequency());
			//DBG(mSampleRate);
			//float a = fft.getFundamentalFrequency();
			//osc.setFreq(fft.getFundamentalFrequency());
			

			if (fft.checkTone() && timeCounter > 150) {
				if (!toneOn) {
					toneOn = true;
					timeCounter = 0;
				}
				else {
					toneOn = false;
					timeCounter = 0;
				}
			}

			auto* channelData = buffer.getReadPointer(channel);

			for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
				fft.pushSampleIntoFifo(channelData[sample]);
				//osc.process(channelData[sample]);
				//channelData[sample] = osc.getWave(osc.WFSqr);
			}
		}

		//on / off sound
		auto* channelData = buffer.getWritePointer(channel);
		if (toneOn)//shutDown sound
			for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
				channelData[sample] = channelData[sample] * Decibels::decibelsToGain(-60.0f);
			}
		//--------------------------------
		
		

		const float* bufferData = buffer.getReadPointer(channel);
		const float* delayBufferData = mDelayBuffer.getReadPointer(channel);

		fillDelayBuffer(channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
		getFromDelayBuffer(buffer, channel, bufferLength, delayBufferLength, bufferData, delayBufferData);

	}
	mWritePosition += bufferLength;
	mWritePosition %= delayBufferLength;

	/*auto chInv = 1.0f / float(buffer.getNumChannels());
	DBG(fft.getFundamentalFrequency());
	for (auto s = 0; s < buffer.getNumSamples(); ++s) {
		auto sample = 0.f;
		for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
			auto * channelData = buffer.getReadPointer(ch, s);
			sample += *channelData;
		}
		sample *= chInv;
		fft.pushSampleIntoFifo(sample);
	}*/
	timeCounter++;
}

void FFTimplAudioProcessor::fillDelayBuffer(int channel, const int bufferLength, const int delayBufferLength,
	const float* bufferData, const float* delayBufferData) {

	//copy data from main buffer to delay buffer
	if (delayBufferLength > bufferLength + mWritePosition) {
		mDelayBuffer.copyFromWithRamp(channel, mWritePosition, bufferData, bufferLength, 0.8, 0.8);
	}
	else {
		const int bufferRemaining = delayBufferLength - mWritePosition;
		mDelayBuffer.copyFromWithRamp(channel, mWritePosition, bufferData, bufferRemaining, 0.8, 0.8);
		mDelayBuffer.copyFromWithRamp(channel, 0, bufferData + bufferRemaining, bufferLength - bufferRemaining, 0.8, 0.8);
	}
}

void FFTimplAudioProcessor::getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, const int bufferLength,
	const int delayBufferLength, const float* bufferData, const float* delayBufferData) {

	int delayTime = mDelay;//ex 500 ms
	const int readPosition = static_cast<int> (delayBufferLength + mWritePosition - (mSampleRate * delayTime / 1000)) % delayBufferLength;

	if (delayBufferLength > bufferLength + readPosition) {

		buffer.copyFrom(channel, 0, delayBufferData + readPosition, bufferLength);
	}
	else {
		const int bufferRemaining = delayBufferLength - readPosition;
		buffer.copyFrom(channel, 0, delayBufferData + readPosition, bufferRemaining);
		buffer.copyFrom(channel, bufferRemaining, delayBufferData, bufferLength - bufferRemaining);
	}
}

//==============================================================================
bool FFTimplAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FFTimplAudioProcessor::createEditor()
{
	return new FFTimplAudioProcessorEditor(*this);
}

//==============================================================================
void FFTimplAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void FFTimplAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new FFTimplAudioProcessor();
}
