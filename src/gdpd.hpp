#ifndef GDPD_H
#define GDPD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Godot.hpp>
#include <AudioStreamPlayer.hpp>

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "RtAudio.h"

namespace godot {

class Gdpd : public godot::AudioStreamPlayer, public pd::PdReceiver {
    GODOT_CLASS(Gdpd, AudioStreamPlayer)

private:
	float *m_inBuf; 
	float *m_outBuf;
	Array* m_messages;
	pd::PdBase m_pd;
	pd::Patch m_patch;
	RtAudio m_audio;
	unsigned int m_bufferFrames;
	float m_vol;
	int m_nbInputs;
	int m_nbOutputs;
	int m_sampleRate;
	int m_inputDevice;
	int m_outputDevice;

public:
    static void _register_methods();

    Gdpd();
    ~Gdpd();

	void _init();

	//libpd functions
	Array get_available_input_devices();
	Array get_available_output_devices();
	int init_devices(String inputDevice, String outputDevice);
	int init(int nbInputs, int nbOutputs, int sampleRate, int bufferSize);
	int start();
	void openfile(String basename, String dirname);
	void closefile();
	bool has_message();
	Array get_next();
	int blocksize();
	void subscribe(String symbStr);
	int start_message(int nbValues);
	void add_symbol(String symbStr);
	void add_float(float val);
	int finish_list(String destStr);

	//libpd hooks
	virtual void print(const std::string& message);
	void receiveList(const std::string& dest, const pd::List& list);

	//godot functions
	void set_volume(float vol);
	inline const float& get_volume(){return m_vol;}


	//rtaudio
	static int audioCallback(void *outputBuffer, void *inputBuffer, 
							 unsigned int nBufferFrames, double streamTime, 
							 RtAudioStreamStatus status, void *userData);
	void processAudio(void *outputBuffer, void *inputBuffer, 
					   unsigned int nBufferFrames, double streamTime, 
					   RtAudioStreamStatus status, void *userData);

};

}

#endif
