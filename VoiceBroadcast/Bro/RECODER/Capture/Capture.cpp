
#include "Capture.h"

#if CAPTURE


CaptureManager::CaptureManager()
{
	TargetLatency = 20;
	TargetDurationInSec = 10; //10초 녹음
}

CaptureManager::~CaptureManager()
{
}


HRESULT CaptureManager::Initialize()
{
	//DWORD dw = (DWORD)dwCoInit;

	/* < CoInitializeEx >
	호출 스레드를 사용하는 COM library를 초기화 하고, 스레드의 동시성 모델을 설정하고,
	필요한 경우 스레드의 apartment를 생성한다.
	*/
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)){
		printf("Unable to initialize COM: %x\n", hr);
		return false;
	}
	//CreateCapturer();
	return true;
}

bool CaptureManager::CreateCapturer(IMMDevice* device)
{
	bool isDefaultDevice = true;
	//ERole role ;
	m_capture_tool = new (std::nothrow) CWASAPICapture(device, isDefaultDevice, m_default_ERole);
	if (m_capture_tool == nullptr) 
	{
		fprintf(stderr,"Unable to allocate capturer\n");
		return false;
	}

	if (m_capture_tool->Initialize(TargetLatency)) 
	{
	    m_immDevice = device;
		return true;
	}
	return false;
}

bool CaptureManager::SaveWaveFile(BYTE *CaptureBuffer, size_t BufferSize, const WAVEFORMATEX *WaveFormat)
{
	wchar_t waveFileName[MAX_PATH];
	HRESULT hr = StringCbCopy(waveFileName, sizeof(waveFileName), L"WASAPICaptureEventDriven-");
	if (SUCCEEDED(hr))
	{
		GUID testGuid;
		if (SUCCEEDED(CoCreateGuid(&testGuid)))
		{
			wchar_t *guidString;
			if (SUCCEEDED(StringFromCLSID(testGuid, &guidString)))
			{
				hr = StringCbCat(waveFileName, sizeof(waveFileName), guidString);
				if (SUCCEEDED(hr))
				{
					hr = StringCbCat(waveFileName, sizeof(waveFileName), L".WAV");
					if (SUCCEEDED(hr))
					{
						HANDLE waveHandle = CreateFile(waveFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
							NULL);
						if (waveHandle != INVALID_HANDLE_VALUE)
						{
							if (WriteWaveFile(waveHandle, CaptureBuffer, BufferSize, WaveFormat))
							{
								printf("Successfully wrote WAVE data to %S\n", waveFileName);
								return true;
							}
							else
							{
								printf("Unable to write wave file\n");
								return false;
							}
							CloseHandle(waveHandle);
						}
						else
						{
							printf("Unable to open output WAV file %S: %d\n", waveFileName, GetLastError());
							return false;
						}
					}
				}
				CoTaskMemFree(guidString);
			}
		}
	}
	return false;
}

bool CaptureManager::WriteWaveFile(HANDLE FileHandle, const BYTE *Buffer, const size_t BufferSize, const WAVEFORMATEX *WaveFormat)
{
	DWORD waveFileSize = sizeof(WAVEHEADER) + sizeof(WAVEFORMATEX) + WaveFormat->cbSize + sizeof(WaveData) + sizeof(DWORD) + static_cast<DWORD>(BufferSize);
	BYTE *waveFileData = new (std::nothrow) BYTE[waveFileSize];
	BYTE *waveFilePointer = waveFileData;
	WAVEHEADER *waveHeader = reinterpret_cast<WAVEHEADER *>(waveFileData);

	if (waveFileData == NULL)
	{
		printf("Unable to allocate %d bytes to hold output wave data\n", waveFileSize);
		return false;
	}

	//
	//  Copy in the wave header - we'll fix up the lengths later.
	//
	CopyMemory(waveFilePointer, WaveHeader, sizeof(WaveHeader));
	waveFilePointer += sizeof(WaveHeader);

	//
	//  Update the sizes in the header.
	//
	waveHeader->dwSize = waveFileSize - (2 * sizeof(DWORD));
	waveHeader->dwFmtSize = sizeof(WAVEFORMATEX) + WaveFormat->cbSize;

	//
	//  Next copy in the WaveFormatex structure.
	//
	CopyMemory(waveFilePointer, WaveFormat, sizeof(WAVEFORMATEX) + WaveFormat->cbSize);
	waveFilePointer += sizeof(WAVEFORMATEX) + WaveFormat->cbSize;


	//
	//  Then the data header.
	//
	CopyMemory(waveFilePointer, WaveData, sizeof(WaveData));
	waveFilePointer += sizeof(WaveData);
	*(reinterpret_cast<DWORD *>(waveFilePointer)) = static_cast<DWORD>(BufferSize);
	waveFilePointer += sizeof(DWORD);

	//
	//  And finally copy in the audio data.
	//
	CopyMemory(waveFilePointer, Buffer, BufferSize);

	//
	//  Last but not least, write the data to the file.
	//
	DWORD bytesWritten;
	if (!WriteFile(FileHandle, waveFileData, waveFileSize, &bytesWritten, NULL))
	{
		printf("Unable to write wave file: %d\n", GetLastError());
		delete []waveFileData;
		return false;
	}

	if (bytesWritten != waveFileSize)
	{
		printf("Failed to write entire wave file\n");
		delete []waveFileData;
		return false;
	}
	delete []waveFileData;
	return true;
}

void CaptureManager::CaptureStart(unsigned char* captureBuffer, size_t captureBufferSize)
{

//	unsigned long long size = strlen(captureBuffer);
	if (m_capture_tool->Start(captureBuffer, captureBufferSize))
	{
		do
		{
			Sleep(1000);
			printf(".");
		} while (--TargetDurationInSec);

		m_capture_tool->Stop();

		//
		//  We've now captured our wave data.  Now write it out in a wave file.
		//
		SaveWaveFile(captureBuffer, m_capture_tool->BytesCaptured(), m_capture_tool->MixFormat());


		//
		//  Now shut down the capturer and release it we're done.
		//
		m_capture_tool->Shutdown();
		SafeRelease(&m_capture_tool);
	}

}

LPWSTR CaptureManager::GetDeviceName(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex)
{
	IMMDevice *device;
	LPWSTR deviceId;
	HRESULT hr;

	hr = DeviceCollection->Item(DeviceIndex, &device);
	if (FAILED(hr))
	{
		printf("Unable to get device %d: %x\n", DeviceIndex, hr);
		return NULL;
	}
	hr = device->GetId(&deviceId);
	if (FAILED(hr))
	{
		printf("Unable to get device %d id: %x\n", DeviceIndex, hr);
		return NULL;
	}

	IPropertyStore *propertyStore;
	hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
	SafeRelease(&device);
	if (FAILED(hr))
	{
		printf("Unable to open device %d property store: %x\n", DeviceIndex, hr);
		return NULL;
	}

	PROPVARIANT friendlyName;
	PropVariantInit(&friendlyName);
	hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
	SafeRelease(&propertyStore);

	if (FAILED(hr))
	{
		printf("Unable to retrieve friendly name for device %d : %x\n", DeviceIndex, hr);
		return NULL;
	}

	wchar_t deviceName[128];
	hr = StringCbPrintf(deviceName, sizeof(deviceName), L"%s (%s)", friendlyName.vt != VT_LPWSTR ? L"Unknown" : friendlyName.pwszVal, deviceId);
	if (FAILED(hr))
	{
		printf("Unable to format friendly name for device %d : %x\n", DeviceIndex, hr);
		return NULL;
	}

	PropVariantClear(&friendlyName);
	CoTaskMemFree(deviceId);

	wchar_t *returnValue = _wcsdup(deviceName);
	if (returnValue == NULL)
	{
		printf("Unable to allocate buffer for return\n");
		return NULL;
	}
	return returnValue;
}

bool CaptureManager::PickDevice(IMMDevice **DeviceToUse, bool *IsDefaultDevice, ERole *DefaultDeviceRole)
{
	HRESULT hr;
	bool retValue = true;
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	IMMDeviceCollection *deviceCollection = NULL;

	*IsDefaultDevice = false;   // Assume we're not using the default device.

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		printf("Unable to instantiate device enumerator: %x\n", hr);
		retValue = false;
		goto Exit;
	}

	IMMDevice *device = NULL;

	//
	//  First off, if none of the console switches was specified, use the console device.
	//
	if (!UseConsoleDevice && !UseCommunicationsDevice && !UseMultimediaDevice && OutputEndpoint == NULL)
	{
		//
		//  The user didn't specify an output device, prompt the user for a device and use that.
		//
		hr = deviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &deviceCollection);
		if (FAILED(hr))
		{
			printf("Unable to retrieve device collection: %x\n", hr);
			retValue = false;
			goto Exit;
		}
		UINT deviceCount;
		hr = deviceCollection->GetCount(&deviceCount);
		if (FAILED(hr))
		{
			printf("Unable to get device collection length: %x\n", hr);
			retValue = false;
			goto Exit;
		}
		for (UINT i = 0 ; i < deviceCount ; i += 1)
		{
			LPWSTR deviceName;

			deviceName = GetDeviceName(deviceCollection, i);
			if (deviceName == NULL)
			{
				retValue = false;
				goto Exit;
			}
			int convertLength = WideCharToMultiByte( CP_ACP, 0, deviceName, -1, NULL, 0, NULL, NULL);
			WideCharToMultiByte( CP_ACP, 0, deviceName, -1, &m_deviceName[0][0], convertLength, NULL, NULL);
			free(deviceName);
		}
		wchar_t choice[10];
		_getws_s(choice);   // Note: Using the safe CRT version of _getws.

		long deviceIndex;
		wchar_t *endPointer;

		deviceIndex = wcstoul(choice, &endPointer, 0);
		if (deviceIndex == 0 && endPointer == choice)
		{
			printf("unrecognized device index: %S\n", choice);
			retValue = false;
			goto Exit;
		}
		switch (deviceIndex)
		{
		case 0:
			UseConsoleDevice = 1;
			break;
		case 1:
			UseCommunicationsDevice = 1;
			break;
		case 2:
			UseMultimediaDevice = 1;
			break;
		default:
			hr = deviceCollection->Item(deviceIndex - 3, &device);
			if (FAILED(hr))
			{
				printf("Unable to retrieve device %d: %x\n", deviceIndex - 3, hr);
				retValue = false;
				//goto Exit;
			}
			break;
		}
	} 
	else if (OutputEndpoint != NULL)
	{
		hr = deviceEnumerator->GetDevice(OutputEndpoint, &device);
		if (FAILED(hr))
		{
			printf("Unable to get endpoint for endpoint %S: %x\n", OutputEndpoint, hr);
			retValue = false;
			goto Exit;
		}
	}

	if (device == NULL)
	{
		ERole deviceRole = eConsole;    // Assume we're using the console role.
		if (UseConsoleDevice)
		{
			deviceRole = eConsole;
		}
		else if (UseCommunicationsDevice)
		{
			deviceRole = eCommunications;
		}
		else if (UseMultimediaDevice)
		{
			deviceRole = eMultimedia;
		}
		hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, deviceRole, &device);
		if (FAILED(hr))
		{
			printf("Unable to get default device for role %d: %x\n", deviceRole, hr);
			retValue = false;
			goto Exit;
		}
		*IsDefaultDevice = true;
		*DefaultDeviceRole = deviceRole;
	}

	*DeviceToUse = device;
	retValue = true;
Exit:
	SafeRelease(&deviceCollection);
	SafeRelease(&deviceEnumerator);

	return retValue;
}


#endif