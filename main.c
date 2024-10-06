#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <libusb-1.0/libusb.h>
#include <pthread.h>
#include "mic_array.h"
#include <signal.h>
#include <unistd.h>
//#define PCM_DEVICE "plughw:1,0"
//#define CAPTURE_DURATION 1
//#define SAMPLE_RATE 44100    // Sampling rate in Hz
//#define PCM_FORMAT SND_PCM_FORMAT_S16_LE  // PCM format (16-bit little-endian)
//#define CAPTURE_DURATION 1

volatile int running = 1;

void handle_sigint(int sig){
    printf("Caught signal %d, cleaning up...\n",sig);
    running = 0;
}

int main(int argc, char *argv[]) {

    signal(SIGINT, handle_sigint);
    char *device_names[MAX_DEVICES] = {0};
    pthread_t mic_threads[7] = {0};
    snd_pcm_t *playback_handle;
    short *mixed_buffer;

    get_device_names(device_names);

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_names[i]) {
            printf("Device %d: %s\n", i + 1, device_names[i]);
            //free(device_names[i]);  // Don't forget to free the allocated memory
        } else {
            printf("Device %d: Not Found\n", i + 1);
        }
    }


//********************************************* */
//Testing Above
///////////////////////////////////////////////////s

    if(argc > 1){
        printf("No arguments needed\n");
        return 0;
    }
    printf("micarray started...\n");
    int myArraySize = sizeof(mic_array_t);
    printf("micarray size is: %d\n",myArraySize);
    mic_array_t *myArray[MAX_DEVICES] = {0};

    for(int i = 0 ; i < MAX_DEVICES ; i++){
        myArray[i] = malloc(sizeof(mic_array_t));
        myArray[i]->device_name = device_names[i];
        myArray[i]->rate = SAMPLE_RATE;
        myArray[i]->micPos = i;
        myArray[i]->thread = mic_threads[i];
        myArray[i]->running = 0;
    }   

    if(open_and_configure_capture_devices(myArray) < 0){
        printf("Error open and configure\n");
        return -1;
    }

    setup_output_pcm(&playback_handle);
    
    mixed_buffer = malloc(FRAMES * PLAYBACK_FRAMES);

    while(running){
        memset(mixed_buffer,0,FRAMES*PLAYBACK_FRAMES);
        capture_audio(myArray, mixed_buffer, PLAYBACK_FRAMES);

        play_audio(mixed_buffer, FRAMES, playback_handle);
    }

    close_capture_devices(myArray);
    snd_pcm_close(playback_handle);
    return 0;
    //if(open_file_and_record(myArray[0]) < 0){
    //    printf("Error recording\n");
    //    return -1;
    //}
/*
    for(int i = 0; i < MAX_DEVICES; i++){
    myArray[i]->running = 1;
    pthread_create(&myArray[i]->thread,NULL,open_file_and_record_thread,myArray[i]);
    //open_file_and_record(myArray[i]);
    }

    //pthread_join(myArray[0]->thread,NULL);

    int threads_complete = 0;

    for(int i = 0 ; i < MAX_DEVICES ; i++){
        threads_complete += myArray[i]->running;
    }
    int running_threads = MAX_DEVICES;
    while(running_threads){
        //printf("running threads: %d\n",running_threads);
        for(int i = 0 ; i < MAX_DEVICES ; i++){
            if(myArray[i]->running == 0){
                printf("thread is running: %d\n",myArray[i]->running);
                myArray[i]->running = -1;
                pthread_join(myArray[i]->thread,NULL);
                close_capture_device(myArray[i]);
                free(device_names[i]);
		        running_threads -= 1;
                //free(myArray[i]);
            }
        }

    }

    if(0){//close_capture_devices(myArray) < 0){
        printf("Error closing devices\n");
        return -1;
    }
    for(int i = 0 ; i < MAX_DEVICES ; i++){
        //free(device_names[i]);
        free(myArray[i]);
    }
    */
 
}
