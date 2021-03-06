/*******************************************************************************
*                                                                              *
*   SDL_ffmpeg is a library for basic multimedia functionality.                *
*   SDL_ffmpeg is based on ffmpeg.                                             *
*                                                                              *
*   Copyright (C) 2007  Arjan Houben                                           *
*                                                                              *
*   SDL_ffmpeg is free software: you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published   *
*	by the Free Software Foundation, either version 3 of the License, or any   *
*   later version.                                                             *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
*   GNU Lesser General Public License for more details.                        *
*                                                                              *
*   You should have received a copy of the GNU Lesser General Public License   *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
*                                                                              *
*******************************************************************************/

#ifndef SDL_FFMPEG_INCLUDED
#define SDL_FFMPEG_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    #include "SDL_thread.h"
    #include "SDL.h"
    #ifdef SDL_FFMPEG_LIBRARY
        #include "ffmpeg/avformat.h"
    #endif
#endif

#ifdef __unix__
    #include "SDL/SDL_thread.h"
    #include "SDL/SDL.h"
    #ifdef SDL_FFMPEG_LIBRARY
        #include "ffmpeg/avformat.h"
    #endif
#endif

#include "stdint.h"

#define SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES      50
#define SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES     512

typedef struct SDL_ffmpegAudioFrame {
    int64_t timestamp;
    uint8_t *buffer;
    uint32_t size;
} SDL_ffmpegAudioFrame;

typedef struct SDL_ffmpegVideoFrame {
    int64_t timestamp;
    SDL_Surface *buffer;
    int filled;
} SDL_ffmpegVideoFrame;

/* this is the basic stream for SDL_ffmpeg */
typedef struct SDL_ffmpegStream {

    /* pointer to ffmpeg data, internal use only!
      points to AVCodecContext */
    int pixFmt;
    void *_ffmpeg;

    /* semaphore for current stream */
    //SDL_sem *sem;
	HANDLE sem;

    /* audio/video buffers */
    SDL_ffmpegAudioFrame audioBuffer[ SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES ];

    SDL_ffmpegVideoFrame videoBuffer[ SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES ];

    /* userinfo */
    double frameRate[2];
    char language[4];
    int sampleRate;
    int channels;
    char codecName[32];
    double timeBase;
    uint16_t width;
    uint16_t height;

    /* extra data for audio */
    int id;
    int64_t lastTimeStamp;
    /* int64_t pts, hardPts;
    int64_t totalBytes;*/

} SDL_ffmpegStream;

typedef struct SDL_ffmpegFile {

    /* pointer to ffmpeg data, internal use only!
       points to AVFormatContext */
    void *_ffmpeg;

    /* our streams */
    SDL_ffmpegStream **vs;
    SDL_ffmpegStream **as;

    /* data used for syncing/searching */
    int64_t offset, videoOffset, startTime;
    int pause;

    /* streams and data about threads */
    int VStreams, AStreams, videoStream, audioStream, threadActive;
    //SDL_Thread *threadID;
	HANDLE threadID;
    //SDL_sem *decode;
	HANDLE decode;

} SDL_ffmpegFile;


int SDL_ffmpegStartDecoding(SDL_ffmpegFile* file);

int SDL_ffmpegStopDecoding(SDL_ffmpegFile* file);

SDL_ffmpegVideoFrame* SDL_ffmpegGetVideoFrame(SDL_ffmpegFile* file);

int SDL_ffmpegReleaseVideo(SDL_ffmpegFile *file, SDL_ffmpegVideoFrame *frame);

SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, int audioID);

int SDL_ffmpegSelectAudioStream(SDL_ffmpegFile* file, int audioID);

SDL_ffmpegStream* SDL_ffmpegGetVideoStream(SDL_ffmpegFile *file, int audioID);

int SDL_ffmpegSelectVideoStream(SDL_ffmpegFile* file, int videoID);

SDL_ffmpegFile* SDL_ffmpegCreateFile();

void SDL_ffmpegFree(SDL_ffmpegFile* file);

SDL_ffmpegFile* SDL_ffmpegOpen(const char* filename);

int SDL_ffmpegDecodeThread(void* data);

int SDL_ffmpegSeek(SDL_ffmpegFile* file, int64_t timestamp);

int SDL_ffmpegSeekRelative(SDL_ffmpegFile* file, int64_t timestamp);

int SDL_ffmpegFlush(SDL_ffmpegFile *file);

SDL_ffmpegAudioFrame* SDL_ffmpegGetAudioFrame(SDL_ffmpegFile *file);

int SDL_ffmpegReleaseAudio(SDL_ffmpegFile *file, SDL_ffmpegAudioFrame *frame, int len);

int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file);

SDL_AudioSpec* SDL_ffmpegGetAudioSpec(SDL_ffmpegFile *file, int samples, void *callback);

int SDL_ffmpegGetVideoSize(SDL_ffmpegFile *file, int *w, int *h);

int64_t SDL_ffmpegGetDuration(SDL_ffmpegFile *file);

int SDL_ffmpegValidAudio(SDL_ffmpegFile *file);

int SDL_ffmpegValidVideo(SDL_ffmpegFile *file);

int SDL_ffmpegPause(SDL_ffmpegFile *file, int state);

int SDL_ffmpegGetState(SDL_ffmpegFile *file);


#ifdef __cplusplus
}
#endif

#endif /* SDL_FFMPEG_INCLUDED */
