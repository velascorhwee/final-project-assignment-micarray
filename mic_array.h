#ifndef MIC_H
#define MIC_H

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#define NUM_MICS 1
#define CHANNELS 1
#define SAMPLE_RATE 44100
#define PCM_FORMAT SND_PCM_FORMAT_S16_LE  // PCM format (16-bit little-endian)
#define CAPTURE_DURATION 5


typedef struct {
    char *device_name;
    char *buffer;
    snd_pcm_t *pcm_handle;
    snd_pcm_uframes_t frames;
    int buffer_size;
    snd_pcm_hw_params_t *params;
    unsigned int rate;
    int dir;
    FILE *recFile;
    pthread_t thread;
    int running;
}   mic_array_t;
  

int open_and_configure_capture_devices(mic_array_t *mic_array);

int close_capture_devices(mic_array_t *mic_array);

int open_file_and_record(mic_array_t *mic_array);

#endif