#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "mic_array.h"
#define PCM_DEVICE "plughw:1,0"
#define CAPTURE_DURATION 5
#define SAMPLE_RATE 44100    // Sampling rate in Hz
#define CHANNELS 1           // Number of channels (1 for mono, 2 for stereo)
#define PCM_FORMAT SND_PCM_FORMAT_S16_LE  // PCM format (16-bit little-endian)

int main(int argc, char *argv[]) {
    if(argc < 3){
        printf("Usage: <card #> <device #>");
        return 0;
    }
    printf("micarray started...\n");
    int myArraySize = sizeof(mic_array_t);
    printf("micarray size is: %d\n",myArraySize);
    mic_array_t myArray[NUM_MICS];
    char device_name[] = "plughw:2,0";
    //strcat(device_name, "plughw:");
    //strcat(device_name,argv[1]);
    //strcat(device_name,",");
    //strcat(device_name,argv[2]);
    printf("Device Name: %s\n",device_name);    
    myArray[0].device_name=device_name;//("plughw:1,0");
    myArray[0].rate = SAMPLE_RATE;
    if(open_and_configure_capture_devices(myArray) < 0){
        printf("Error open and configure\n");
        return -1;
    }

    if(open_file_and_record(myArray) < 0){
        printf("Error recording\n");
        return -1;
    }

    if(close_capture_devices(myArray) < 0){
        printf("Error closing devices\n");
        return -1;
    }

    return 0;
}
