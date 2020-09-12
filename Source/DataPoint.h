/*
  ==============================================================================

    DataPoint.h
    Created: 20 Aug 2020 12:54:43pm
    Author:  Throw

  ==============================================================================
*/

#pragma once
class DataPoint {

public: DataPoint(int songId, int time) {
		this->songId = songId;
		this->time = time;
	}
		//needed default constructor to serialize with nlohmann json
		DataPoint() {}

	int getTime() const {
		return time;
	}

	int getSongId() const {
		return songId;
	}

	void setTime(int time) {
		this->time = time;
	}

	void setSongId(int songId) {
		this->songId = songId;
	}

private:
	int time;
	int songId;

};
