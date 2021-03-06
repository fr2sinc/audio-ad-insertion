/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdInsertionAudioProcessor::AdInsertionAudioProcessor()
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
	initializeFprint();	
}

void AdInsertionAudioProcessor::initializeFprint() {

	//first of all setup fingerprint
	//set your host samplerate manually
	fprintLive.setupFingerprintLive(44100);
	JsonUtility::readHashMap(fprintLive.getHashMap());
	JsonUtility::readJingleMap(fprintLive.getJingleMap());
	fprint.setupFingerprint(44100, 10/*seconds of audio to analyze*/);
	JsonUtility::readHashMap(fprint.getHashMap());
	JsonUtility::readJingleMap(fprint.getJingleMap());
}

AdInsertionAudioProcessor::~AdInsertionAudioProcessor()
{
}

//==============================================================================
const juce::String AdInsertionAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool AdInsertionAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool AdInsertionAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool AdInsertionAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double AdInsertionAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int AdInsertionAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int AdInsertionAudioProcessor::getCurrentProgram()
{
	return 0;
}

void AdInsertionAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String AdInsertionAudioProcessor::getProgramName(int index)
{
	return {};
}

void AdInsertionAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void AdInsertionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
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
	delayInSamples = sampleRate * 6; //6sec of fixed delay
}

void AdInsertionAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
	transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AdInsertionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void AdInsertionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	const int bufferLength = buffer.getNumSamples();
	const int delayBufferLength = mDelayBuffer.getNumSamples();

	//changeToneState();
	changeFprintState(bufferLength);

	for (int channel = 0; channel < totalNumInputChannels; ++channel) {

		const float* bufferData = buffer.getReadPointer(channel);
		const float* delayBufferData = mDelayBuffer.getReadPointer(channel);

		//doToneAnalysis(channel, bufferLength, bufferData);
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
		//fade out of Ad Injected (if Ad has a duration longer than 15 s)
		if (samplesAdRemaining > 0 && (samplesAdDuration/mSampleRate) > 15) {			
			buffer.applyGain(smoothed.getNextValue());
		}
		//fprint offline
		//fprint.pushSampleIntoSongMatchFifo(buffer, bufferLength);
	}

	mWritePosition += bufferLength;
	mWritePosition %= delayBufferLength;
	timeCounter++;
}

void AdInsertionAudioProcessor::doFprintAnalysis(int channel, const int bufferLength, juce::AudioBuffer<float> buffer) {
	auto* bufferData = buffer.getReadPointer(0);
	if (channel == 0) {

		if (fState == fSearchingJ1) {
			
			for (int sample = 0; sample < bufferLength; ++sample) {
#ifdef MatchMapOverlap
				RecognizedJingle localRJ = fprintLive.getRecognitionWithMatchMapOverlap(bufferData[sample]);
#else
				RecognizedJingle localRJ = fprintLive.getRecognitionWithOverlap(bufferData[sample]);
#endif // MatchMapOverlap			
				if (localRJ.getRemainingInSamples() > 0) {
					fState = fRecognizedJ1;
					injState1 = waitJ1End;

					samplesRemainingDelayedJ1 = delayInSamples + localRJ.getRemainingInSamples() - (bufferLength - sample);
					j1SamplesRemaining = localRJ.getRemainingInSamples() - (bufferLength - sample);
					curJingle1 = localRJ.getTitle();
					loadInjectionFile();
					break;
				}

			}
		}
		else if (fState == fSearchingJ2) {
			
			for (int sample = 0; sample < bufferLength; ++sample) {

				samplesAdCounter++;
				//stop looking for j2 if it's been more than 3 minutes
				if ((samplesAdCounter / mSampleRate) > 180) {
					fState = fRecognizedJ2;
					injState2 = waitAdEnd;

					samplesAdRemaining = 3 * mSampleRate;
					samplesAdDuration = samplesAdCounter;

					j2SamplesRemaining = 3 * mSampleRate;					
					curJingle2 = "TimeOut";					
					samplesRemainingDelayedJ2 = samplesAdRemaining;

					//fade out setup
					smoothed.setCurrentAndTargetValue(1.0);
					smoothed.reset(samplesAdRemaining / bufferLength);
					smoothed.setTargetValue(0.0);
					samplesAdCounter = 0;
					break;
				}

#ifdef MatchMapOverlap
				RecognizedJingle localRJ = fprintLive.getRecognitionWithMatchMapOverlap(bufferData[sample]);
#else
				RecognizedJingle localRJ = fprintLive.getRecognitionWithOverlap(bufferData[sample]);
#endif // MatchMapOverlap			
				if (localRJ.getOffsetInSamples() > 0) {
					fState = fRecognizedJ2;
					injState2 = waitAdEnd;
										
					samplesAdRemaining = delayInSamples - localRJ.getOffsetInSamples();					
					samplesAdDuration = samplesAdCounter + samplesAdRemaining;

					//fade out setup
					smoothed.setCurrentAndTargetValue(1.0);					
					smoothed.reset(samplesAdRemaining / bufferLength);
					smoothed.setTargetValue(0.0);

					second_jingle_duration = localRJ.getDurationInSamples();
					samplesRemainingDelayedJ2 = samplesAdRemaining + second_jingle_duration;
					j2SamplesRemaining = localRJ.getRemainingInSamples() - (bufferLength - sample);

					curJingle2 = localRJ.getTitle();
					samplesAdCounter = 0;
					break;
				}

			}
		}
		
	}
}

//tester with no trigger
/*void AdInsertionAudioProcessor::doFprintAnalysis(int channel, const int bufferLength, juce::AudioBuffer<float> buffer) {
	auto* bufferData = buffer.getReadPointer(0);
	if (channel == 0) {
		//fingerprint
		for (int sample = 0; sample < bufferLength; ++sample) {			
				fprintLive.getRecognitionWithOverlap(bufferData[sample]);
		}
	}
}*/

void AdInsertionAudioProcessor::doToneAnalysis(int channel, const int bufferLength,	const float* bufferData) {

	//calculate FFT only in one channel, if I need to calculate fft on two channel I have to create e mono channel from two channel in a trivial way
	if (channel == 0) {		

		if (timeCounter > 150) { //10ms * 100 in samples
			bool fftResult = gAnalyzer.checkGoertzelFrequencies();
			if (fftResult) {
				newToneState = On;
			}
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

void AdInsertionAudioProcessor::changeFprintState(const int bufferLength) {

	if (fState == fRecognizedJ1) {
		if (j1SamplesRemaining - bufferLength > 0) {
			j1SamplesRemaining -= bufferLength;
		}
		else { 			
			fState = fSearchingJ2;
			j1SamplesRemaining = 0;			
		}
	}
	else if (fState == fRecognizedJ2) {		
		if (j2SamplesRemaining - bufferLength > 0) {
			j2SamplesRemaining -= bufferLength;			
		}
		else { 		
			fState = fSearchingJ1;	
		}
	}
		
	if (injState1 == waitJ1End) {		
		if (samplesRemainingDelayedJ1 - bufferLength > 0) {
			samplesRemainingDelayedJ1 -= bufferLength;
		}
		else {		
			toneState = On;
			samplesRemainingDelayedJ1 = 0;
			injState1 = notActiveInj1;
		}
	}

	if (injState2 == waitAdEnd) {
		if (samplesAdRemaining - bufferLength > 0) {
			samplesAdRemaining -= bufferLength;
			samplesRemainingDelayedJ2 -= bufferLength;
		}
		else {
			toneState = Off;		
			samplesAdDuration = 0;
			samplesAdRemaining = 0;
			injState2 = waitJ2;
		}
	}
	else if (injState2 == waitJ2){
		if (second_jingle_duration - bufferLength > 0) {
			second_jingle_duration -= bufferLength;
			samplesRemainingDelayedJ2 -= bufferLength;
		}
		else {			
			second_jingle_duration = 0;
			samplesRemainingDelayedJ2 = 0;
			injState2 = notActiveInj2;
		}
	}

}

void AdInsertionAudioProcessor::changeToneState() {

	if (toneState == On) {
		if (newToneState == On) {
			toneState = Off;
			timeCounter = 0;
		}
	}
	else if (toneState = Off) {
		if (newToneState == On) {
			toneState = On;			
			loadInjectionFile();
			timeCounter = 0;
		}
	}
}

void AdInsertionAudioProcessor::loadInjectionFile() {

	//load random audio files from audioInjection directory
	File dir = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioInjection");
	Array<File> injFiles = dir.findChildFiles(2, ".mp3; .wav");

	if (injFiles.size() != 0) {

		Random rand = Random();
		int x = rand.nextInt(Range<int>(0, injFiles.size() - 1));
		File file = injFiles[x];
		auto* reader = formatManager.createReaderFor(file);
		if (reader != nullptr) {
			std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
			transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
			readerSource.reset(newSource.release());
		}
		transportSource.start();
	}

}

void AdInsertionAudioProcessor::fillDelayBuffer(int channel, const int bufferLength, const int delayBufferLength,
	const float* bufferData, const float* delayBufferData) {

	//copy data from main buffer to delay buffer
	if (delayBufferLength > bufferLength + mWritePosition) {
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferLength);
	}
	else {
		const int bufferRemaining = delayBufferLength - mWritePosition;
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferRemaining);
		mDelayBuffer.copyFrom(channel, 0, bufferData + bufferRemaining, bufferLength - bufferRemaining);
	}
}

void AdInsertionAudioProcessor::getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, const int bufferLength,
	const int delayBufferLength, const float* bufferData, const float* delayBufferData) {

	//int delayTime = 6000;//mDelay;//ex 500 ms
	//(mSampleRate * delayTime / 1000)
	const int readPosition = static_cast<int> (delayBufferLength + mWritePosition - delayInSamples) % delayBufferLength;

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
bool AdInsertionAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AdInsertionAudioProcessor::createEditor()
{
	return new AdInsertionAudioProcessorEditor(*this);
}

//==============================================================================
void AdInsertionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void AdInsertionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new AdInsertionAudioProcessor();
}
