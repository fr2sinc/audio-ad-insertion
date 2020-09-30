/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleSignalGeneratorAudioProcessor::SimpleSignalGeneratorAudioProcessor()
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
}

SimpleSignalGeneratorAudioProcessor::~SimpleSignalGeneratorAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleSignalGeneratorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleSignalGeneratorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleSignalGeneratorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleSignalGeneratorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleSignalGeneratorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleSignalGeneratorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleSignalGeneratorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleSignalGeneratorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleSignalGeneratorAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleSignalGeneratorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleSignalGeneratorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	amplitude = 0.5;
	frequency = 500;
	phase = 0.0;
	time = 0.0;
	deltaTime = 1 / sampleRate;
}

void SimpleSignalGeneratorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleSignalGeneratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleSignalGeneratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
  
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	if (time >= std::numeric_limits<float>::max()) 
		time = 0.0;	
	
	if (mute) {
		buffer.clear();
	}
	else {
		float *monoBuffer = new float[buffer.getNumSamples()];

		//generate mono sin wave
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
			monoBuffer[sample] = amplitude * sin(2 * (double)M_PI * frequency * time + phase);
			time += deltaTime;
		}
		//from mono to stereo		
		for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
			buffer.copyFrom(channel, 0, monoBuffer, buffer.getNumSamples());
		}
		delete[] monoBuffer;
	}	
}

//==============================================================================
bool SimpleSignalGeneratorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleSignalGeneratorAudioProcessor::createEditor()
{
    return new SimpleSignalGeneratorAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleSignalGeneratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleSignalGeneratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleSignalGeneratorAudioProcessor();
}
