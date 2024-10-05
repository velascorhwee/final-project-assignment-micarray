#include "mic_array.h"

int open_and_configure_capture_devices(mic_array_t **mic_array){
    int rc;
    unsigned int rate = SAMPLE_RATE;
    int dir;
    

    
    for(int i = 0; i < MAX_DEVICES; i ++){
       // Open the PCM device for capture (recording)
    int rc = snd_pcm_open(&mic_array[i]->pcm_handle, mic_array[i]->device_name, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(rc));
        return -1;
    }
    
    // Allocate a hardware parameters object
    snd_pcm_hw_params_malloc(&mic_array[i]->params);

    // Fill it in with default values
    snd_pcm_hw_params_any(mic_array[i]->pcm_handle, mic_array[i]->params);

    // Set the desired hardware parameters

    // Interleaved mode
    snd_pcm_hw_params_set_access(mic_array[i]->pcm_handle, mic_array[i]->params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Set the sample format
    snd_pcm_hw_params_set_format(mic_array[i]->pcm_handle, mic_array[i]->params, PCM_FORMAT);

    // Set the number of channels
    snd_pcm_hw_params_set_channels(mic_array[i]->pcm_handle, mic_array[i]->params, CHANNELS);

    // Set the sampling rate
    snd_pcm_hw_params_set_rate_near(mic_array[i]->pcm_handle, mic_array[i]->params, &mic_array[i]->rate, &mic_array[i]->dir);

    // Set period size to 32 frames
    mic_array[i]->frames = 32;
    snd_pcm_hw_params_set_period_size_near(mic_array[i]->pcm_handle, mic_array[i]->params, &mic_array[i]->frames, &mic_array[i]->dir);

    // Write the parameters to the driver
    rc = snd_pcm_hw_params(mic_array[i]->pcm_handle, mic_array[i]->params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(rc));
        return -1;
    }

    // Use a buffer large enough to hold one period
    snd_pcm_hw_params_get_period_size(mic_array[i]->params, &mic_array[i]->frames, &mic_array[i]->dir);
    mic_array[i]->buffer_size = mic_array[i]->frames * CHANNELS * 2; // 2 bytes per sample (16-bit)
    mic_array[i]->buffer = (char *) malloc((mic_array[i]->buffer_size));
    printf("Mic configured\n");
    

    }

    return 1;
}

int close_capture_device(mic_array_t *micarray){
    printf("clean up mic %d\n",micarray->micPos);
    free(micarray->buffer);
    if(micarray->recFile != NULL){
        fclose(micarray->recFile);
    }
    snd_pcm_drain(micarray->pcm_handle);
    snd_pcm_close(micarray->pcm_handle);
    snd_pcm_hw_params_free(micarray->params);

    return 1;
}

int close_capture_devices(mic_array_t **mic_array){
    for(int i = 0; i < MAX_DEVICES; i++){
        // Clean up
    printf("clean up time %d\n",i);
    free(mic_array[i]->buffer);
    printf("about to close files\n");
    if(mic_array[i]->recFile != NULL){
        fclose(mic_array[i]->recFile);
    }
    printf("about to drain\n");
    snd_pcm_drain(mic_array[i]->pcm_handle);
    printf("about to close\n");
    snd_pcm_close(mic_array[i]->pcm_handle);
    printf("about to free params\n");
    snd_pcm_hw_params_free(mic_array[i]->params);
    
    }

    return 1;
}

void *open_file_and_record_thread(void *arg){
    mic_array_t *mic = (mic_array_t *)arg;

    // Open the output file
    char filename[256] = "capture";
    char append[16] = {0};
    sprintf(append,"%d",mic->micPos);
    strcat(filename,append);
    strcat(filename,".wav");
    mic->recFile = fopen(filename, "wb");
    if (mic->recFile == NULL) {
        fprintf(stderr, "Unable to open output file\n");
        return (void *)-1;
    }
    int rc;
    // Capture data for the specified duration
    int loops = (CAPTURE_DURATION * SAMPLE_RATE) / (mic->frames);
    
    printf("About to record Loops: %d \n", loops);
    printf("Frames = %lu, Buffer Size:%d\n",(mic->frames), (mic->buffer_size));
    while (loops > 0) {
        loops--;
        rc = snd_pcm_readi(mic->pcm_handle, mic->buffer, (mic->frames));
        //printf("RC: %d\n",rc);
        if (rc == -EPIPE) {
            // EPIPE means overrun
            fprintf(stderr, "Overrun occurred\n");
            snd_pcm_prepare(mic->pcm_handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error from read: %s %d\n", snd_strerror(rc), mic->micPos);
        } else if (rc != (int)(mic->frames)) {
            fprintf(stderr, "Short read, read %d frames\n", rc);
        }
        fwrite(mic->buffer, 1, mic->buffer_size, mic->recFile);
    }
    printf("Finished recording \n");

    mic->running = 0;
    pthread_exit(NULL);

}

int open_file_and_record(mic_array_t *mic){
    // Open the output file
    char filename[256] = "capture";
    char append[16] = {0};
    sprintf(append,"%d",mic->micPos);
    strcat(filename,append);
    strcat(filename,".wav");
    mic->recFile = fopen(filename, "wb");
    if (mic->recFile == NULL) {
        fprintf(stderr, "Unable to open output file\n");
        return -1;
    }
    int rc;
    // Capture data for the specified duration
    int loops = (CAPTURE_DURATION * SAMPLE_RATE) / (mic->frames);
    
    printf("About to record Loops: %d \n", loops);
    printf("Frames = %lu, Buffer Size:%d\n",(mic->frames), (mic->buffer_size));
    while (loops > 0) {
        loops--;
        rc = snd_pcm_readi(mic->pcm_handle, mic->buffer, (mic->frames));
        //printf("RC: %d\n",rc);
        if (rc == -EPIPE) {
            // EPIPE means overrun
            fprintf(stderr, "Overrun occurred\n");
            snd_pcm_prepare(mic->pcm_handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error from read: %s\n", snd_strerror(rc));
        } else if (rc != (int)(mic->frames)) {
            fprintf(stderr, "Short read, read %d frames\n", rc);
        }
        fwrite(mic->buffer, 1, mic->buffer_size, mic->recFile);
    }
    printf("Finished recording \n");
    
    return 1;
}

void print_device_names(){
    void **hints;
    int err;
    char *name, *desc, *io;

    // Get device hints
    err = snd_device_name_hint(-1, "pcm", &hints);
    if (err < 0) {
        fprintf(stderr, "Error getting device hints: %s\n", snd_strerror(err));
        return;
    }

    // Iterate over the list of hints
    void **n = hints;
    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if (io != NULL && strcmp(io, "Input") == 0 && name != NULL){
        if (name != NULL) {
            printf("Device Name: %s\n", name);
        }

        if (desc != NULL) {
            printf("Description: %s\n", desc);
        }

        if (io != NULL) {
            printf("IO Type: %s\n", io);
        }

        printf("\n");
        }
        if (name)
            free(name);
        if (desc)
            free(desc);
        if (io)
            free(io);

        n++;
    }

    // Free the hints array
    snd_device_name_free_hint(hints);
}

void get_device_names(char **devices){
    FILE *file = fopen("/proc/asound/cards", "r");
    if (!file) {
        perror("Error opening /proc/asound/cards");
        return;
    }

    char line[256];
    int current_card = -1;
    while (fgets(line, sizeof(line), file)) {
        // Check if it's a card description line by looking for the card number at the start of the line
        if (sscanf(line, "%d", &current_card) == 1) {
            continue;  // Skip to next line after storing card number
        }

        // Check if the line contains a USB device description
        char *usb_pos = strstr(line, "usb-");
        if (usb_pos) {
            // Extract the USB port number (e.g., 1.1.3)
            char port[10];
            if (sscanf(usb_pos, "usb-%*[^-]-%9s", port) == 1) {
                // Isolate the third part of the port (e.g., 1.1.3 -> "1.3")
                char *third_part = strchr(port, '.');
                printf("third part : %s\n",third_part);
                //this is very specific for the raspberry pi 4 using the TP Link 7 port usb hub
                if (third_part && strlen(third_part) >= 2) {
                    int index = -1;
                    if (strcmp(third_part, ".2.1.1,") == 0) index = 0;
                    else if (strcmp(third_part, ".2.1.2,") == 0) index = 1;
                    else if (strcmp(third_part, ".2.1.3,") == 0) index = 2;
                    else if (strcmp(third_part, ".2.1.4,") == 0) index = 3;
                    else if (third_part[3] == '2') index = 4;
                    else if (third_part[3] == '3') index = 5;
                    else if (third_part[3] == '4') index = 6;

                    // If index is valid and within bounds, set the ALSA device name in the array
                    if (index >= 0 && index < MAX_DEVICES) {
                        char alsa_device[16];
                        snprintf(alsa_device, sizeof(alsa_device), "plughw:%d,0", current_card);

                        // Allocate memory for the device name and copy it into the array
                        devices[index] = malloc(strlen(alsa_device) + 1);
                        strcpy(devices[index], alsa_device);
                    }
                }
            }
        }
    }

    fclose(file);
}