/*
  ==============================================================================

    JsonUtility.cpp
    Created: 12 Sep 2020 11:10:06am
    Author:  Throw

  ==============================================================================
*/

#include "JsonUtility.h"


//==============================================================================
JsonUtility::JsonUtility()
{    
}

JsonUtility::~JsonUtility()
{
}

void to_json(json& j, const DataPoint& p) {
	j = json{ {"songId", p.getSongId()}, {"time", p.getTime()} };
}

void from_json(const json& j, DataPoint& p) {

	int songId = j.at("songId");	
	p.setSongId(songId);
	int time = j.at("time");
	p.setTime(time);

}

void JsonUtility::writeHashMap(std::unordered_map<long long, std::list<DataPoint>>& hashMap) {
	json j(hashMap);

	File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\dataStructures\\hashMap.txt");
	if (f.exists()) {
		f.deleteFile();
	}
	f.create();
	f.appendText(j.dump());
}

void JsonUtility::readHashMap(std::unordered_map<long long, std::list<DataPoint>>& hashMap) {
	hashMap.clear();

	File fileToRead = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\dataStructures\\hashMap.txt");
	if (fileToRead.existsAsFile()) {
		std::string fileText = fileToRead.loadFileAsString().toStdString();
		json j = json::parse(fileText);
		hashMap = j.get<std::unordered_map<long long, std::list<DataPoint>>>();
	}	
}

void JsonUtility::writeJingleMap(std::unordered_map<int, std::pair<int, std::string>>& jingleMap) {
	json j(jingleMap);

	File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\dataStructures\\jingleMap.txt");
	if (f.exists()) {
		f.deleteFile();
	}
	f.create();
	f.appendText(j.dump());
}

void JsonUtility::readJingleMap(std::unordered_map<int, std::pair<int, std::string>>& jingleMap) {
	jingleMap.clear();

	File fileToRead = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\dataStructures\\jingleMap.txt");
	if (fileToRead.existsAsFile()) {
		std::string fileText = fileToRead.loadFileAsString().toStdString();
		json j = json::parse(fileText);
		jingleMap = j.get<std::unordered_map<int, std::pair<int, std::string>>>();
	}
}
