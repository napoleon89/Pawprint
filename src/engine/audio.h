#ifndef AUDIO_H
#define AUDIO_H

struct SoundEffect {
	
	
	void loadFromMem() {
		
		
	}
};

struct AudioEngine {
	
	
	virtual void init();
	virtual void playSound();
	virtual void uninit();
};

#endif // AUDIO_H