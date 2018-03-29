#pragma once
#include "stdafx.h"

#define CAPTURE 1
#if CAPTURE 

#include <Functiondiscoverykeys_devpkey.h>

#include <stdio.h>
#include <string>

#include "defines.h"
//#include "CmdLine.h"
#include "WASAPICapture.h"


class CaptureManager;
class FileSaver;

class BroadcastManager
{
public: 
	BroadcastManager(){}
	~BroadcastManager(){}

	//CaptureManager* capturer;
	//RTSPSender* sender;
	//Encoder* encoder;

	virtual bool Init(std::string CameraInfo[]);
	virtual bool Start(){return false;}
	virtual bool Stop() {return false;}

private:

};

class CaptureManager
{
public:
	CaptureManager();
	virtual ~CaptureManager();
	
	IMMDevice* m_immDevice;
	ERole m_default_ERole;
	CWASAPICapture* m_capture_tool;

	HRESULT Initialize(); 
	void CaptureStart(unsigned char* captureBuffer, size_t captureBufferSize);
	
	std::string GetDeviceName() const { return m_deviceName[0]; }
	size_t GetBufferSize()
	{
		return m_capture_tool->SamplesPerSecond() * TargetDurationInSec * m_capture_tool->FrameSize();
	}
	bool PickDevice(IMMDevice **DeviceToUse, bool *IsDefaultDevice, ERole *DefaultDeviceRole);
	bool CreateCapturer(IMMDevice* device);
private:
	//<file>
	bool SaveWaveFile(BYTE *CaptureBuffer, size_t BufferSize, const WAVEFORMATEX *WaveFormat);
	bool WriteWaveFile(HANDLE FileHandle, const BYTE *Buffer, const size_t BufferSize, const WAVEFORMATEX *WaveFormat);
	//<\file>
	LPWSTR GetDeviceName(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex);

private:
	std::string m_deviceName[16]; //15∞≥ ¿”Ω√
	
	int TargetLatency;
	int TargetDurationInSec;
	bool ShowHelp;
	bool UseConsoleDevice;
	bool UseCommunicationsDevice;
	bool UseMultimediaDevice;
	bool DisableMMCSS;
	wchar_t *OutputEndpoint;
};



class FileSaver 
{
public:
	FileSaver(){}
	~FileSaver(){}

	//bool SaveWaveFile(BYTE *CaptureBuffer, size_t BufferSize, const WAVEFORMATEX *WaveFormat);
	//bool WriteWaveFile(HANDLE FileHandle, const BYTE *Buffer, const size_t BufferSize, const WAVEFORMATEX *WaveFormat);
private:

};




#endif