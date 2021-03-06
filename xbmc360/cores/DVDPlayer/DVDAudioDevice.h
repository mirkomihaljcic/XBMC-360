#ifndef H_CDVDAUDIODEVICE
#define H_CDVDAUDIODEVICE

#include <xtl.h>
#include <xaudio2.h>

struct SSoundData
{
	int iSize;
	void* pVoid; 
};

class CDVDAudio : public IXAudio2VoiceCallback
{
public:
	CDVDAudio();
	~CDVDAudio();

	bool Create(int iChannels, int iBitrate, int iBitsPerSample);
	void Destroy();

	DWORD AddPackets(unsigned char* data, DWORD len);
	void Flush();
	void Pause();
	void Resume();
	int GetBytesInBuffer();
	__int64 GetDelay();

	// XAudio2 Callbacks
	void OnStreamEnd() { SetEvent( m_hBufferEndEvent ); }
	void OnVoiceProcessingPassEnd() {}
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
	void OnBufferStart(void * pBufferContext) {}
	void OnBufferEnd(void * pBufferContext);
	void OnLoopEnd(void * pBufferContext) {}
	void OnVoiceError(void * pBufferContext, HRESULT Error) {}

private:
	bool m_bInitialized;
	HANDLE m_hBufferEndEvent;
	CRITICAL_SECTION m_CriticalSection;

    IXAudio2* m_pXAudio2;
	IXAudio2SourceVoice* m_pSourceVoice;

	int m_iBufferSize;

	int m_iBitrate;
	int m_iChannels;
};

#endif //H_CDVDAUDIODEVICE