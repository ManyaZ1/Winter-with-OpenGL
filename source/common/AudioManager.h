#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#ifdef AM_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#endif

#include "../external/miniaudio.h"
#include <map>
#include <string>
#include <iostream>

class AudioManager {
public:
    ma_engine engine;
    // Map of string keys to ma_sound pointers
    std::map<std::string, ma_sound*> soundLibrary;

    inline AudioManager() {
        if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
            std::cerr << "Failed to initialize audio engine" << std::endl;
        }
    }

    inline ~AudioManager() {
        // C++11 compatible iterator loop (Fixes 'name' and 'sound' undefined)
        for (auto it = soundLibrary.begin(); it != soundLibrary.end(); ++it) {
            ma_sound* pSound = it->second;
            if (pSound != nullptr) {
                ma_sound_uninit(pSound);
                delete pSound; // This is now safe as pSound is explicitly a pointer
            }
        }
        ma_engine_uninit(&engine);
    }

    inline void preload(const std::string& name, const std::string& filePath) {
        ma_sound* newSound = new ma_sound();

        // MA_SOUND_FLAG_DECODE: loads into memory for instant access
        ma_result result = ma_sound_init_from_file(&engine, filePath.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, newSound);

        if (result == MA_SUCCESS) {
            soundLibrary[name] = newSound;
        }
        else {
            std::cerr << "Audio Error: Could not load " << filePath << std::endl;
            delete newSound;
        }
    }

    inline void playPreloaded(const std::string& name, bool loop = false) {
        if (soundLibrary.find(name) != soundLibrary.end()) {
            ma_sound* pSound = soundLibrary[name];
            ma_sound_set_looping(pSound, loop ? MA_TRUE : MA_FALSE);
            ma_sound_start(pSound);
        }
    }

    // Fire-and-forget for one-off sounds (Simultaneous play)
    inline void playEffect(const std::string& filePath) {
        ma_engine_play_sound(&engine, filePath.c_str(), NULL);
    }
    // Add this inside your AudioManager class in AudioManager.h

    inline void stopPreloaded(const std::string& name) {
        if (soundLibrary.find(name) != soundLibrary.end()) {
            ma_sound_stop(soundLibrary[name]);

            // Optional: Rewind the sound to the start so it plays 
            // from the beginning next time you call playPreloaded
            ma_sound_seek_to_pcm_frame(soundLibrary[name], 0);
        }
    }

    inline void stopAll() {
        // This tells the entire engine to stop processing audio
        ma_engine_stop(&engine);

        // To restart the engine later (if needed)
        // ma_engine_start(&engine);
    }
};
extern AudioManager audio;
#endif