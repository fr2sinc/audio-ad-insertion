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
	formatManager.registerBasicFormats();       // [1]	
	initializeFprint(fprintLive);
	//JsonUtility::writeHashMap(fprintLive.getHashMap());
	//JsonUtility::writeJingleMap(fprintLive.getJingleMap());
	JsonUtility::readHashMap(fprintLive.getHashMap());
	JsonUtility::readJingleMap(fprintLive.getJingleMap());

}

void FFTimplAudioProcessor::initializeFprint(FingerprintLive& fprint) {

	//first of all setup fingerprint
	//set your host samplerate manually
	fprint.setupFingerprintLive(48000, 0.5);

	//---------------------------------------------------------
	/*File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioDatabase");
	DirectoryIterator dir_iterator(f, false);

	int songId = 0;
	while (dir_iterator.next()) {

		auto file = File(dir_iterator.getFile());
		fprint.loadHashes(songId, false, file.getFullPathName());
		songId++;
	}*/
	//---------------------------------------------------------
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
	const int delayBufferSize = 10 * (sampleRate + samplesPerBlock); //10sec of delay buffer
	mDelayBuffer.setSize(numInputChannels, delayBufferSize);

	gAnalyzer.setSampleRate(sampleRate);
	transportSource.prepareToPlay(samplesPerBlock, sampleRate);
}

void FFTimplAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
	transportSource.releaseResources();

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

	changeToneState();

	if (fState == fOn) {
		//meglio anticipare che posticipare
		if (samplesRemaining - bufferLength > 0) {
			samplesRemaining -= bufferLength;
		}
		else { //<=0
			curJingle = "";
			fState = fMiddle;
			toneState = On;
			samplesRemaining = 0;
		}
	}

	if (fState == fMiddleAtivated) {
		//meglio anticipare che posticipare
		if (sampleAdRemaining - bufferLength > 0) {
			sampleAdRemaining -= bufferLength;
		}
		else { //<=0
			//curJingle = "";
			fState = fWaitSecondJingle;
			toneState = Off;
			sampleAdRemaining = 0;
		}
	}

	if (fState == fWaitSecondJingle) {
		//meglio anticipare che posticipare
		if (second_jingle_duration - bufferLength > 0) {
			second_jingle_duration -= bufferLength;
		}
		else { //<=0
			//curJingle = "";
			fState = fOff;//può riprendere a riconoscere il primo jingle			
			second_jingle_duration = 0;
		}
	}

	for (int channel = 0; channel < totalNumInputChannels; ++channel) {

		const float* bufferData = buffer.getReadPointer(channel);
		const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
		
		doToneAnalysis(channel, bufferLength, bufferData);
		doFprintAnalysis(channel, bufferLength, buffer);			

		//delay actions
		fillDelayBuffer(channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
		getFromDelayBuffer(buffer, channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
				
	}

	//audio injection from audioFile into main Buffer, one time for all channels
		//I take advantage from delay actions
	if (toneState == On /*&& channel == 0*/) {
		//injection
		transportSource.getNextAudioBlock(AudioSourceChannelInfo(buffer));
		//fprint.pushSamplesIntoSongFifo(buffer, bufferLength);
	}

	

	mWritePosition += bufferLength;
	mWritePosition %= delayBufferLength;
	timeCounter++;
}

void FFTimplAudioProcessor::doFprintAnalysis(int channel, const int bufferLength, juce::AudioBuffer<float> buffer) {
	auto* bufferData = buffer.getReadPointer(0);
	if (channel == 0) {

		if (fState == fOff) {

			//fingerprint
			for (int sample = 0; sample < bufferLength; ++sample) {

				RecognizedJingle localRJ = fprintLive.getRecognitionSamplesOffset(bufferData[sample]);
				//handle negative case, it's a critical case
				if (localRJ.getRemainingInSamples() > 0) {
					fState = fOn;
					//set current recognition
					samplesRemaining = (6 * mSampleRate) + localRJ.getRemainingInSamples() - (bufferLength - sample);
					curJingle = localRJ.getTitle();

					File file = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioInjection\\1.mp3");
					auto* reader = formatManager.createReaderFor(file);

					if (reader != nullptr) {
						std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
						transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
						readerSource.reset(newSource.release());
					}
					transportSource.start();
					break;
				}

			}
		}
		else if (fState == fMiddle) {

			//fingerprint
			for (int sample = 0; sample < bufferLength; ++sample) {

				RecognizedJingle localRJ = fprintLive.getRecognitionSamplesOffset(bufferData[sample]);
				
				if (localRJ.getOffsetInSamples() > 0) {
					sampleAdRemaining = (6 * mSampleRate) - localRJ.getOffsetInSamples();
					second_jingle_duration = localRJ.getDurationInSamples();
					//toneState = Off;
					fState = fMiddleAtivated;
				}

			}
		}
		
	}
}

//void FFTimplAudioProcessor::doFprintAnalysis(int channel, const int bufferLength, juce::AudioBuffer<float> buffer) {
//	auto* bufferData = buffer.getReadPointer(0);
//	if (channel == 0) {
//		//fingerprint
//		for (int sample = 0; sample < bufferLength; ++sample) {			
//				int localSamples = fprintLive.pushSampleIntoSongMatchFifo(bufferData[sample]);
//		}
//	}
//}

void FFTimplAudioProcessor::doToneAnalysis(int channel, const int bufferLength,	const float* bufferData) {

	//calculate FFT only in one channel, if I need to calculate fft on two channel I have to create e mono channel from two channel in a trivial way
	if (channel == 0) {

		bool fftResult = gAnalyzer.checkDetectDTMF();

		if (fftResult && timeCounter > 150) {
			newToneState = On;
		}
		else {
			newToneState = Off;
		}		

		//fill fifo buffer to perform after frequency analysis
		for (int sample = 0; sample < bufferLength; ++sample) {
			gAnalyzer.pushSampleIntoFifo(bufferData[sample]);			
		}

	}
}

void FFTimplAudioProcessor::changeToneState() {

	if (toneState == On) {
		if (newToneState == On) {
			toneState = Off;
			transportSource.stop();
			timeCounter = 0;
		}
	}
	else if (toneState = Off) {
		if (newToneState == On) {
			toneState = On;

			File file = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioInjection\\1.mp3");
			auto* reader = formatManager.createReaderFor(file);

			if (reader != nullptr) {
				std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
				transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
				readerSource.reset(newSource.release());
			}			
			transportSource.start();
			
			timeCounter = 0;
		}
	}
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

	int delayTime = 6000;//mDelay;//ex 500 ms
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
