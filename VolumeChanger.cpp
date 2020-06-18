#include "stdafx.h"
#include <stdio.h>

using namespace::Microsoft::WRL;
using namespace::Microsoft::WRL::Details;

class ComInitializer
{
private:
	HRESULT m_hr;
public:
	ComInitializer() { m_hr = CoInitialize(nullptr); }
	ComInitializer(LPVOID pvReserved) { m_hr = CoInitialize(pvReserved); }
	~ComInitializer() { CoUninitialize(); }
	operator HRESULT() const { return m_hr; }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	UINT loopDelay;

	int nArgs = 0;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if (NULL == szArglist || nArgs != 2)
		loopDelay = 200;
	else
		loopDelay = _wtoi(szArglist[1]);

	ComInitializer initializer;
	if (FAILED(initializer))
		return -1;

	// Get device enumeration object
	ComPtr<IMMDeviceEnumerator> deviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(initializer))
		RaiseException(hr);

	// Get the default multimedia output device
	ComPtr<IMMDevice> device;
	hr = deviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eRender, ERole::eMultimedia, &device);
	if (FAILED(initializer))
		RaiseException(hr);

	// Create a volume object for the audio endpoint
	ComPtr<IAudioEndpointVolume> audioEndpointVolume;
	hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, &audioEndpointVolume);
	if (FAILED(hr))
		RaiseException(hr);

	// Get information up to jack
	ComPtr<IDeviceTopology> pDeviceTopology;
	ComPtr<IConnector> pConnEP;
	IConnector* pConnDeviceTo;
	ComPtr<IPart> pPart;
	device->Activate(__uuidof(IDeviceTopology), CLSCTX_INPROC_SERVER, NULL, (void**)&pDeviceTopology);
	pDeviceTopology->GetConnector(0, &pConnEP);
	pConnEP->GetConnectedTo(&pConnDeviceTo);
	pConnDeviceTo->QueryInterface(__uuidof(IPart), (void**)&pPart);

	ComPtr<IKsJackDescription> pJackDesc = NULL;
	UINT deviceIdNew = 0;
	UINT deviceIdOld = 0;
	float headphoneVolume = 0;
	float speakerVolume = 0;

	// Read current value
	audioEndpointVolume->GetMasterVolumeLevelScalar(&headphoneVolume);

	// Main loop
	while (1)
	{
		pPart->Activate(CLSCTX_INPROC_SERVER, __uuidof(IKsJackDescription), &pJackDesc);
		pJackDesc->GetJackCount(&deviceIdNew);

		// Output changed
		if (deviceIdNew != deviceIdOld)
		{
			if (deviceIdNew > deviceIdOld)
			{
				hr = audioEndpointVolume->GetMasterVolumeLevelScalar(&speakerVolume);
				hr = audioEndpointVolume->SetMasterVolumeLevelScalar(headphoneVolume, nullptr);
			}
			else
			{
				hr = audioEndpointVolume->GetMasterVolumeLevelScalar(&headphoneVolume);
				hr = audioEndpointVolume->SetMasterVolumeLevelScalar(speakerVolume, nullptr);
			}

			deviceIdOld = deviceIdNew;
		}
		Sleep(loopDelay);
	}

	return 0;
}
