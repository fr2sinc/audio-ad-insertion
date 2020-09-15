/*
  ==============================================================================

    RecognizedJingle.cpp
    Created: 15 Sep 2020 9:41:07pm
    Author:  Throw

  ==============================================================================
*/


#include "RecognizedJingle.h"

//==============================================================================
RecognizedJingle::RecognizedJingle(): durationInSamples(-1), offsetInSamples(-1), remainingInSamples(-1), id(-1), title(""){
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
}

RecognizedJingle::RecognizedJingle(int duration, int offset, int remaining, int id, std::string title) {
	this->durationInSamples = duration;
	this->offsetInSamples = offset;
	this->remainingInSamples = remaining;
	this->id = id;
	this->title = title;
}

RecognizedJingle::RecognizedJingle(const RecognizedJingle & source) {

	this->durationInSamples = source.getDurationInSamples();
	this->offsetInSamples = source.getOffsetInSamples();
	this->remainingInSamples = source.getRemainingInSamples();
	this->id = source.getId();
	this->title = source.getTitle();
}

RecognizedJingle::~RecognizedJingle() {
}

RecognizedJingle& RecognizedJingle::operator=(const RecognizedJingle& source) {

	if (this != &source) {
		this->durationInSamples = source.getDurationInSamples();
		this->offsetInSamples = source.getOffsetInSamples();
		this->remainingInSamples = source.getRemainingInSamples();
		this->id = source.getId();
		this->title = source.getTitle();
	}	
	return *this;
}

void RecognizedJingle::setDurationInSamples(int duration) {
	this->durationInSamples = duration;
}

void RecognizedJingle::setOffsetInSamples(int offset) {
	this->offsetInSamples = offset;
}

void RecognizedJingle::setRemainingInSamples(int remaining) {
	this->remainingInSamples = remaining;
}

void RecognizedJingle::setId(int id) {
	this->id = id;
}

void RecognizedJingle::setTitle(std::string title) {
	this->title = title;
}

int RecognizedJingle::getDurationInSamples() const
{
	return durationInSamples;
}

int RecognizedJingle::getOffsetInSamples() const
{
	return offsetInSamples;
}

int RecognizedJingle::getRemainingInSamples() const
{
	return remainingInSamples;
}

int RecognizedJingle::getId() const
{
	return id;
}

std::string RecognizedJingle::getTitle() const
{
	return this->title;
}

bool RecognizedJingle::isEmpty()
{
	if (this->durationInSamples != -1)
		return false;
	else
		return true;
}
