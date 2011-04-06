#ifndef _CHUNKER_PLAYER_H
#define _CHUNKER_PLAYER_H

#include "player_defines.h"
#include <SDL.h>
#include <SDL_mutex.h>

#ifdef PSNR_PUBLICATION
#include <repoclient.h>
#endif

typedef struct SChannel
{
	char LaunchString[255];
	char Title[255];
	int Width;
	int Height;
	float Ratio;
	int Bitrate;
	int SampleRate;
	short int AudioChannels;
	int Index;
	//quality related info
	double instant_score; //updated continuously
	double average_score; //updated on a long term time window (i.e. 2 minutes)
	double score_history[CHANNEL_SCORE_HISTORY_SIZE];
	int history_index;
	char quality[255];
	int startTime;
} SChannel;

SDL_mutex *OverlayMutex;
SDL_Overlay *YUVOverlay;
SDL_Rect OverlayRect;
SDL_Surface *MainScreen;
int SilentMode;
int queue_filling_threshold;
int quit;
short int QueueFillingMode;
int LogTraces;
char NetworkID[255];
char RepoAddress[2048];

#ifdef PSNR_PUBLICATION
HANDLE repoclient;
struct timeval LastTimeRepoPublish;
#endif

#ifdef EMULATE_CHUNK_LOSS
typedef struct
{
	time_t Time;
	unsigned int Value;
	unsigned int MinValue;
	unsigned int MaxValue;
	unsigned int Burstiness;
} SChunkLoss;
SChunkLoss* ScheduledChunkLosses;
int CurrChunkLossIndex, NScheduledChunkLosses;
#endif

void* P2PProcessHandle;

#ifdef __WIN32__
//~ #define KILL_PROCESS(pid) {char command_name[255]; sprintf(command_name, "taskkill /pid %d /F", pid); system(command_name);}
#define KILL_PROCESS(handle) {TerminateProcess(((PROCESS_INFORMATION*)handle)->hProcess, 0);}
#define KILLALL(pname) {char command_name[255]; sprintf(command_name, "taskkill /im %s /F", pname); system(command_name);}
//#endif
//#ifdef __LINUX__
#else
#define KILL_PROCESS(handle) {if(*((pid_t*)handle)>0){char command_name[255]; sprintf(command_name, "kill %d", *((pid_t*)handle)); system(command_name);}}
#define KILLALL(pname) {char command_name[255]; sprintf(command_name, "killall %s -9", pname); system(command_name);}
#endif
//#ifdef __MACOS__
//#define KILL_PROCESS(handle) {if(*((pid_t*)handle)>0){char command_name[255]; sprintf(command_name, "kill %d", *((pid_t*)handle)); system(command_name);}}
//#define KILLALL(pname) {char command_name[255]; sprintf(command_name, "killall %s -9", pname); system(command_name);}
//#endif

#ifdef __LINUX__
#define DELETE_DIR(folder) {char command_name[255]; sprintf(command_name, "rm -fR %s", folder); system(command_name); }
#define CREATE_DIR(folder) {char command_name[255]; sprintf(command_name, "mkdir %s", folder); system(command_name); }
#else
#define DELETE_DIR(folder) {char command_name[255]; sprintf(command_name, "rd /S /Q %s", folder); system(command_name); }
#define CREATE_DIR(folder) {char command_name[255]; sprintf(command_name, "mkdir %s", folder); system(command_name); }
#endif

SChannel Channels[MAX_CHANNELS_NUM];
int NChannels;
int SelectedChannel;
char StreamerFilename[255];
int FullscreenMode; // fullscreen vs windowized flag
int window_width, window_height;
int Audio_ON;

#ifdef HTTPIO
int HttpPort;
#else
#ifdef TCPIO
int TcpPort;
#endif
#endif

int CheckForRepoAddress(char* Param);
void ZapDown();
void ZapUp();
int ParseConf();
int SwitchChannel(SChannel* channel);
int ReTune(SChannel* channel);

#endif // _CHUNKER_PLAYER_H
