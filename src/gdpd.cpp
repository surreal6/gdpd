#include "gdpd.hpp"

using namespace godot;

void Gdpd::_register_methods() {
    register_method("get_available_input_devices", 
					&Gdpd::get_available_input_devices);
    register_method("get_available_output_devices", 
					&Gdpd::get_available_output_devices);
    register_method("init_devices", &Gdpd::init_devices);
    register_method("init", &Gdpd::init);
    register_method("stop", &Gdpd::stop);
    register_method("openfile", &Gdpd::openfile);
    register_method("closefile", &Gdpd::closefile);
    register_method("subscribe", &Gdpd::subscribe);
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
	//create message array
	m_messages = new Array();
	m_init=false;
}

void Gdpd::_init() {

}

Gdpd::~Gdpd() {
}

Array Gdpd::get_available_input_devices() {
	Array gdlist;
	for(int d=0; d<m_audio.getDeviceCount(); d++) {
		if(m_audio.getDeviceInfo(d).inputChannels>0) {
			gdlist.push_back(m_audio.getDeviceInfo(d).name.c_str());
		}
	}
	return gdlist;
}

Array Gdpd::get_available_output_devices() {
	Array gdlist;
	for(int d=0; d<m_audio.getDeviceCount(); d++) {
		if(m_audio.getDeviceInfo(d).outputChannels>0) {
			gdlist.push_back(m_audio.getDeviceInfo(d).name.c_str());
		}
	}
	return gdlist;
}


int Gdpd::init_devices(String inputDevice, String outputDevice) {
	std::wstring inpWs = inputDevice.unicode_str();
	std::string inpStr(inpWs.begin(), inpWs.end());
	std::wstring outWs = outputDevice.unicode_str();
	std::string outStr(outWs.begin(), outWs.end());

	for(int d=0; d<m_audio.getDeviceCount(); d++) {
		std::string n = m_audio.getDeviceInfo(d).name;
		if(n.compare(inpStr)==0) {
			m_inputDevice = d;
		}
		if(n.compare(outStr)==0) {
			m_outputDevice = d;
		}
	}

	RtAudio::DeviceInfo inpInfo = m_audio.getDeviceInfo(m_inputDevice);
	RtAudio::DeviceInfo outInfo = m_audio.getDeviceInfo(m_outputDevice);
	m_nbInputs = inpInfo.inputChannels;
	m_nbOutputs = outInfo.outputChannels;
	m_sampleRate = outInfo.preferredSampleRate;
	m_bufferFrames = 128;
	return start();
}

int Gdpd::init(int nbInputs, int nbOutputs, int sampleRate, int bufferSize) {
	m_inputDevice = m_audio.getDefaultInputDevice();
	m_outputDevice = m_audio.getDefaultOutputDevice();
	RtAudio::DeviceInfo inpInfo = m_audio.getDeviceInfo(m_inputDevice);
	RtAudio::DeviceInfo outInfo = m_audio.getDeviceInfo(m_outputDevice);
	m_nbInputs 	= std::min<int>(nbInputs, inpInfo.inputChannels);
	m_nbOutputs = std::min<int>(nbOutputs, outInfo.outputChannels);
	m_sampleRate = sampleRate;
	m_bufferFrames = std::max<int>(64, bufferSize);
	return start();
}

int Gdpd::start() {
	RtAudio::StreamParameters outParams, inpParams;
	inpParams.deviceId = m_inputDevice;
	inpParams.nChannels = m_nbInputs;
	outParams.deviceId = m_outputDevice;
	outParams.nChannels = m_nbOutputs;
	print("Output channels = "+std::to_string(outParams.nChannels));
	print("Input channels = "+std::to_string(inpParams.nChannels));


	if(!m_pd.init(m_nbInputs, m_nbOutputs, m_sampleRate, true)) {
		print("GDPD : Error starting libpd");
		return 1;
	}

	//libpd_set_verbose(1);

	//start dsp
	m_pd.computeAudio(true);

	//intialize rtaudio
	if(m_audio.getDeviceCount()==0){
		Godot::print("There are no available sound devices.");
	}

	RtAudio::StreamOptions options;
	options.streamName = "gdpd";
	options.flags = RTAUDIO_SCHEDULE_REALTIME;
	if(m_audio.getCurrentApi() != RtAudio::MACOSX_CORE) {
		options.flags |= RTAUDIO_MINIMIZE_LATENCY;
	}
	try {
		m_audio.openStream(&outParams, &inpParams, RTAUDIO_FLOAT32, 
						   m_sampleRate, &m_bufferFrames, &audioCallback, 
						   this, &options);
		m_audio.startStream();
		print("Stream started");
	}
	catch(RtAudioError& e) {
		Godot::print(e.getMessage().c_str());
	}

	//create message hook
	m_pd.subscribe("to_gdpd");
	m_pd.setReceiver(this);
	m_init=true;

	print("Initialized");

	return 0;
}

void Gdpd::stop() {
	m_audio.stopStream();
	m_audio.closeStream();
	m_pd.computeAudio(false);
	print("Stopped");
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

	if(m_patchsMap.find(baseS)!=m_patchsMap.end()) {
		print("Patch "+baseS+" already opened");
		return;
	}

	//libpd_openfile(baseS.c_str(), dirS.c_str());
	//m_patch = m_pd.openPatch(baseS.c_str(), dirS.c_str());
	pd::Patch p1 = m_pd.openPatch(baseS.c_str(), dirS.c_str());
	if(!p1.isValid()) {
		print("Could not open patch "+baseS);
	}
	else {
		print("Opened patch "+baseS);
		m_patchsMap[baseS] = p1;
	}

	//m_pd.subscribe("to_gdpd");

	/*
	if(!m_pd.init(m_nbInputs, m_nbOutputs, m_sampleRate, true)) {
		Godot::print("GDPD : Error starting libpd");
	}
	m_pd.setReceiver(this);
	m_pd.computeAudio(true);
	*/
}

void Gdpd::closefile(godot::String baseStr) {
	std::wstring baseWs = baseStr.unicode_str();
	std::string baseS(baseWs.begin(), baseWs.end());
	if(m_patchsMap.find(baseS)!=m_patchsMap.end()) {
		m_pd.closePatch(m_patchsMap[baseS]);
		m_patchsMap.erase(baseS);
		print("Closed patch "+baseS);
	}
	//m_pd.closePatch(baseS.c_str());
}

void Gdpd::subscribe(String symbStr) {
	std::wstring symbWs = symbStr.unicode_str();
	std::string symbS(symbWs.begin(), symbWs.end());
	m_pd.subscribe(symbS.c_str());
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

