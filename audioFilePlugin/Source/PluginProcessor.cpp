#include "PluginProcessor.h"
#include "PluginEditor.h"
//#define CHOOSER
//==============================================================================
AudioFilePluginAudioProcessor::AudioFilePluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	formatManager.registerBasicFormats();       // [1]	

#ifdef CHOOSER
	juce::File f = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\stream");
	juce::DirectoryIterator dir_iterator(f, false);

	juce::FileChooser chooser("Select an audio file to play...", {}, "*.wav;*.mp3");

	if (chooser.browseForFileToOpen()) {
		auto file = chooser.getResult();

		auto* reader = formatManager.createReaderFor(file);

		if (reader != nullptr) {
			std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
			transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
			readerSource.reset(newSource.release());
		}
	}
#else
	juce::File file = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\stream\\1.wav");
	auto* reader = formatManager.createReaderFor(file);
	if (reader != nullptr) {
		std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
		transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
		readerSource.reset(newSource.release());
	}
#endif // CHOOSER
}

AudioFilePluginAudioProcessor::~AudioFilePluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioFilePluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioFilePluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioFilePluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioFilePluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioFilePluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioFilePluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioFilePluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioFilePluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioFilePluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioFilePluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioFilePluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	transportSource.prepareToPlay(samplesPerBlock, sampleRate);	
	transportSource.start();
}

void AudioFilePluginAudioProcessor::releaseResources()
{
	transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioFilePluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
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

void AudioFilePluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

	transportSource.getNextAudioBlock(juce::AudioSourceChannelInfo(buffer));
}

//==============================================================================
bool AudioFilePluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioFilePluginAudioProcessor::createEditor()
{
    return new AudioFilePluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioFilePluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AudioFilePluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioFilePluginAudioProcessor();
}
