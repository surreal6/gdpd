#include "gdpd.hpp"

using namespace godot;

void Gdpd::_register_methods() {
    register_method("init", &Gdpd::init);
    register_method("openfile", &Gdpd::openfile);
    register_method("closefile", &Gdpd::closefile);
    register_method("has_message", &Gdpd::has_message);
    register_method("get_next", &Gdpd::get_next);
    register_method("start_message", &Gdpd::start_message);
    register_method("add_symbol", &Gdpd::add_symbol);
    register_method("add_float", &Gdpd::add_float);
    register_method("finish_list", &Gdpd::finish_list);
    register_method("set_volume", &Gdpd::set_volume);
}

int Gdpd::audioCallback(void *outputBuffer, void *inputBuffer, 
					    unsigned int nBufferFrames, double streamTime, 
						RtAudioStreamStatus status, void *userData){
	Gdpd* gdpd = static_cast<Gdpd*>(userData);
	gdpd->processAudio(outputBuffer, inputBuffer, nBufferFrames, streamTime,
					   status, userData);
	return 0;
}

Gdpd::Gdpd(): m_vol(1) {
}

void Gdpd::_init() {

}

Gdpd::~Gdpd() {
}

int Gdpd::init(int nbInputs, int nbOutputs, int sampleRate, int bufferSize) {

	if(!m_pd.init(nbInputs, nbOutputs, sampleRate, true)) {
		Godot::print("GDPD : Error starting libpd");
		return 1;
	}

	//libpd_set_verbose(1);

	//create message array
	m_messages = new Array();

	//create message hook
	m_pd.subscribe("to_gdpd");
	m_pd.setReceiver(this);

	//start dsp
	m_pd.computeAudio(true);

	//intialize rtaudio
	if(m_audio.getDeviceCount()==0){
		Godot::print("There are no available sound devices.");
	}

	RtAudio::StreamParameters outParams, inParams;
	unsigned int sr = m_audio.getDeviceInfo(outParams.deviceId).preferredSampleRate;
	outParams.deviceId = m_audio.getDefaultOutputDevice();
	inParams.deviceId = m_audio.getDefaultOutputDevice();
	outParams.nChannels = m_nbInputs = nbInputs;
	inParams.nChannels = m_nbOutputs = nbOutputs;
	m_bufferFrames = bufferSize;

	RtAudio::StreamOptions options;
	options.streamName = "gdpd";
	options.flags = RTAUDIO_SCHEDULE_REALTIME;
	if(m_audio.getCurrentApi() != RtAudio::MACOSX_CORE) {
		options.flags |= RTAUDIO_MINIMIZE_LATENCY; // CoreAudio doesn't seem to like this
	}
	try {
		m_audio.openStream(&outParams, &inParams, RTAUDIO_FLOAT32, 
						   sr, &m_bufferFrames, &audioCallback, 
						   this, &options);
		m_audio.startStream();
	}
	catch(RtAudioError& e) {
		Godot::print(e.getMessage().c_str());
	}


	print("Initialized");

	return 0;
}

void Gdpd::processAudio(void *outputBuffer, void *inputBuffer, 
						unsigned int nBufferFrames, double streamTime, 
						RtAudioStreamStatus status, void *userData) {
	int ticks = nBufferFrames / libpd_blocksize();

	m_pd.processFloat(ticks, (float*)inputBuffer, (float*)outputBuffer);

	//volume control on the output
	for(int b=0; b<nBufferFrames*m_nbOutputs; ++b) {
		((float*)outputBuffer)[b]*=m_vol;
	}
}

void Gdpd::openfile(godot::String baseStr, godot::String dirStr) {
	std::wstring baseWs = baseStr.unicode_str();
	std::string baseS(baseWs.begin(), baseWs.end());
	std::wstring dirWs = dirStr.unicode_str();
	std::string dirS(dirWs.begin(), dirWs.end());

	libpd_openfile(baseS.c_str(), dirS.c_str());

	print("Opened patch");
}

void Gdpd::closefile() {
	m_pd.closePatch(m_patch);
}

bool Gdpd::has_message() {
	//receive new messages
	m_pd.receiveMessages();

	//return if more than one message
	int size = m_messages->size();
    return size>0;
}

Array Gdpd::get_next() {
	Array msg = m_messages->pop_front();
	return msg;
}

int Gdpd::blocksize() {
	int blocksize = libpd_blocksize();
	return blocksize;
}

int Gdpd::start_message(int nbValues) {
    int res = libpd_start_message(nbValues);
	return res;
}

void Gdpd::add_symbol(String symbStr) {
	std::wstring symbWs = symbStr.unicode_str();
	std::string symbS(symbWs.begin(), symbWs.end());
	libpd_add_symbol(symbS.c_str());
}

void Gdpd::add_float(float val) {
	libpd_add_float(val);
}

int Gdpd::finish_list(String destStr) {
	std::wstring destWs = destStr.unicode_str();
	std::string destS(destWs.begin(), destWs.end());
    int res = libpd_finish_list(destS.c_str());
    return res;
}


void Gdpd::print(const std::string& message) {
	Godot::print((std::string("GDPD : ")+message).c_str());
}

void Gdpd::receiveList(const std::string& dest, const pd::List& list) {
	Array gdlist;

	for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i)) {
			gdlist.push_back(list.getFloat(i));
		}
		else if(list.isSymbol(i)) {
			String symbStr(list.getSymbol(i).c_str());
			gdlist.push_back(symbStr);
		}
	}

	m_messages->push_back(gdlist);
}

void Gdpd::set_volume(float vol) {
	m_vol=vol;
}

