#ifndef REALPLAYINGSTATE_H
#define REALPLAYINGSTATE_H

#include "playingstate.h"

class SaveFile;

class RealPlayingState: public PlayingState {
public:
    RealPlayingState(std::unique_ptr<SaveFile> save);
    virtual ~RealPlayingState();

	void start_from_map(const std::string& starting_map);
	bool load_room(const std::string& path, bool use_default_player);
	void make_subsave();
	void load_most_recent_subsave();

private:
	std::unique_ptr<SaveFile> savefile_;
};


#endif // REALPLAYINGSTATE_H
