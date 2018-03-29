#include "Capture.h"


#if CAPTURE

int main()
{
	CaptureManager* CapMan = new CaptureManager();
	
	CapMan->Initialize();
	/*HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		printf("Unable to instantiate device enumerator: %x\n", hr);
	}*/

	IMMDevice* device;
	bool default_ =true;
	
	ERole role;
	CapMan->PickDevice(&device,&default_,&role);
	CapMan->CreateCapturer(device);

	size_t bufferSize = CapMan->GetBufferSize();
	BYTE *captureBuffer = new (std::nothrow) BYTE[bufferSize];

	if (captureBuffer == nullptr)
	{
		printf("Unable to allocate capture buffer\n");
		return false;
	}

	CapMan->CaptureStart(captureBuffer, bufferSize );
	//	CaptureStart( captureBuffer,captureBufferSize);
	if (captureBuffer){
		delete []captureBuffer;
	}
	if (CapMan){
		delete CapMan;
	}
	return 0;
}
#endif