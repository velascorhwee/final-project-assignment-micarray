#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "mic_array.h"
#define PCM_DEVICE "plughw:1,0"
#define CAPTURE_DURATION 5
#define SAMPLE_RATE 44100    // Sampling rate in Hz
#define CHANNELS 1           // Number of channels (1 for mono, 2 for stereo)
#define PCM_FORMAT SND_PCM_FORMAT_S16_LE  // PCM format (16-bit little-endian)


    
int configure_device(char *pcm_name, snd_pcm_t **pcm_handle, snd_pcm_hw_params_t **params,unsigned int *rate, int *dir, snd_pcm_uframes_t *frames, int *buffer_size, char **buffer){

    // Open the PCM device for capture (recording)
    int rc = snd_pcm_open(pcm_handle, pcm_name, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(rc));
        return 1;
    }
    
    // Allocate a hardware parameters object
    snd_pcm_hw_params_malloc(params);

    // Fill it in with default values
    snd_pcm_hw_params_any(*pcm_handle, *params);

    // Set the desired hardware parameters

    // Interleaved mode
    snd_pcm_hw_params_set_access(*pcm_handle, *params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Set the sample format
    snd_pcm_hw_params_set_format(*pcm_handle, *params, PCM_FORMAT);

    // Set the number of channels
    snd_pcm_hw_params_set_channels(*pcm_handle, *params, CHANNELS);

    // Set the sampling rate
    snd_pcm_hw_params_set_rate_near(*pcm_handle, *params, rate, dir);

    // Set period size to 32 frames
    *frames = 32;
    snd_pcm_hw_params_set_period_size_near(*pcm_handle, *params, frames, dir);

    // Write the parameters to the driver
    rc = snd_pcm_hw_params(*pcm_handle, *params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(rc));
        return 1;
    }

    // Use a buffer large enough to hold one period
    snd_pcm_hw_params_get_period_size(*params, frames, dir);
    *buffer_size = *frames * CHANNELS * 2; // 2 bytes per sample (16-bit)
    *buffer = (char *) malloc((*buffer_size));
    printf("Mic configured\n");
    return 1;
}

int open_and_record(FILE **output_file, snd_pcm_t *pcm_handle, snd_pcm_uframes_t *frames, snd_pcm_hw_params_t *params, char *buffer, int *buffer_size){
    // Open the output file
    *output_file = fopen("capture.wav", "wb");
    if (*output_file == NULL) {
        fprintf(stderr, "Unable to open output file\n");
        return 1;
    }
    int rc;
    // Capture data for the specified duration
    int loops = (CAPTURE_DURATION * SAMPLE_RATE) / (*frames);
    
    printf("About to record Loops: %d \n", loops);
    printf("Frames = %lu, Buffer Size:%d\n",(*frames), (*buffer_size));
    while (loops > 0) {
        loops--;
        rc = snd_pcm_readi(pcm_handle, buffer, (*frames));
        //printf("RC: %d\n",rc);
        if (rc == -EPIPE) {
            // EPIPE means overrun
            fprintf(stderr, "Overrun occurred\n");
            snd_pcm_prepare(pcm_handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error from read: %s\n", snd_strerror(rc));
        } else if (rc != (int)(*frames)) {
            fprintf(stderr, "Short read, read %d frames\n", rc);
        }
        fwrite(buffer, 1, *buffer_size, *output_file);
    }
    printf("Finished recording \n");

    return 1;
}

int clean_up(char *buffer, FILE **output_file, snd_pcm_t *pcm_handle, snd_pcm_hw_params_t *params){
// Clean up
    printf("clean up time\n");
    free(buffer);
    fclose(*output_file);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    snd_pcm_hw_params_free(params);
    return 1;
}

int main() {

    mic_array_t myArray[NUM_MICS];

    myArray->device_name=strdup("plughw:1,0");
    myArray->rate = SAMPLE_RATE;
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
#if 0
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = SAMPLE_RATE;
    int rc;
    int dir;
    snd_pcm_uframes_t frames;
    char *buffer;
    int buffer_size;
    FILE *output_file;
    char *pcm_name;

    pcm_name=strdup("plughw:1,0");

    //printf("File Pointer: %d\n",(*output_file));
    configure_device(pcm_name, &pcm_handle,&params,&rate,&dir,&frames,&buffer_size,&buffer);
    if(pcm_handle == NULL){
        printf("PCM handle null\n");
        return -1;
    }
    if(buffer == NULL){
        printf("Buffer is null\n");
        return -1;
    }

    //printf("File Pointer: %d\n",(*output_file));
    open_and_record(&output_file,pcm_handle,&frames,params,buffer,&buffer_size);
/*
    printf("clean up time\n");
    free(buffer);
    printf("freed buffer\n");
    if(output_file == NULL){
        printf("Output File is NULL\n");
        return -1;
    }

    //printf("File Pointer: %d\n",(*output_file));
    if(output_file != NULL){

    
    if(fclose(output_file) < 0){
        printf("error closing file\n");
        printf("Errno is %d\n", errno);
        printf("Errno Description is : %s\n",strerror(errno));
    }
    }
    printf("Cloed file\n");
    snd_pcm_drain(pcm_handle);
    printf("drained pcm handle\n");
    snd_pcm_close(pcm_handle);
    printf("closed pcm handle\n");
    snd_pcm_hw_params_free(params);
    printf("freed hw params\n");*/
    clean_up(buffer,&output_file,pcm_handle,params);


    

    printf("Audio capture complete. Output saved to capture.wav\n");
#endif

#if 0
    //char *buffer;
    int buffer_size;
    FILE *output_file;
    //snd_pcm_uframes_t frames = 32;
    mic_array_t myArray[NUM_MICS];

    int rc;
    printf("Starting program\n");
    
    myArray[0].device_name = strdup("plughw:1,0");
    myArray[1].device_name = strdup("plughw:2,0");

    myArray[0].frames = 32;
    myArray[1].frames = 32;

    printf("Device 0 Name: %s\n", myArray[0].device_name);
    printf("Device 1 Name: %s\n", myArray[1].device_name);

    printf("About to open from device Buffer Size: %d, Frames: %d\n", myArray[0].buffer_size, myArray[0].frames);
    printf("about to open devices\n");
    if(open_and_configure_capture_devices(myArray) < 0){
        printf("Failed opening devices\n");
        return (-1);
    }

    printf("Finished opeing devices\n");
    //pcm_name = strdup("default");

    //snd_pcm_hw_params_alloca(&params);  

    

    output_file = fopen("capture.wav", "wb");
    if(!output_file){
        fprintf(stderr, "Unable to open output file\n");
        return (-1);
    }
    printf("Finished opening file\n");

    int loops = (CAPTURE_DURATION*SAMPLE_RATE)/myArray[0].frames;

    size_t numWriten = 0;

    printf("About to read from device Buffer Size: %d, Frames: %d\n", myArray[0].buffer_size, myArray[0].frames);
    while(loops > 0){
        loops--;
        rc = snd_pcm_readi(myArray[0].pcm_handle, myArray[0].buffer, myArray[0].frames);
        if(rc == -EPIPE){
            printf("Overrun occured\n");
            snd_pcm_prepare(myArray[0].pcm_handle);
        } else if (rc < 0) {
            printf("Error from read: %s\n", snd_strerror(rc));
        } else if (rc != (int)myArray[0].frames){
            printf("Short read, read %d frames\n", rc);
        }
        numWriten = fwrite(myArray[0].buffer, 1, myArray[0].buffer_size, output_file);
        //printf("Wrote %d bytes\n");
    }
    
    //free(buffer);

    printf("Closing files\n");
    if(fclose(output_file) < 0){
        printf("error closing file\n");
        return (-1);
    }
    
    printf("Closing devices\n");
    if(close_capture_devices(myArray) < 0){
        printf("Error closing device\n");
        return (-1);
    }

    printf("Audio capture completed.  Output saved to capture.wav\n");
    return 0;
#endif
}