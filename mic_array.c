#include "mic_array.h"

int open_and_configure_capture_devices(mic_array_t *mic_array){
    int rc;
    unsigned int rate = SAMPLE_RATE;
    int dir;
    

    
    for(int i = 0; i < NUM_MICS; i ++){
       // Open the PCM device for capture (recording)
    int rc = snd_pcm_open(&mic_array[i].pcm_handle, mic_array[i].device_name, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(rc));
        return -1;
    }
    
    // Allocate a hardware parameters object
    snd_pcm_hw_params_malloc(&mic_array[i].params);

    // Fill it in with default values
    snd_pcm_hw_params_any(mic_array[i].pcm_handle, mic_array[i].params);

    // Set the desired hardware parameters

    // Interleaved mode
    snd_pcm_hw_params_set_access(mic_array[i].pcm_handle, mic_array[i].params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Set the sample format
    snd_pcm_hw_params_set_format(mic_array[i].pcm_handle, mic_array[i].params, PCM_FORMAT);

    // Set the number of channels
    snd_pcm_hw_params_set_channels(mic_array[i].pcm_handle, mic_array[i].params, CHANNELS);

    // Set the sampling rate
    snd_pcm_hw_params_set_rate_near(mic_array[i].pcm_handle, mic_array[i].params, &mic_array[i].rate, &mic_array[i].dir);

    // Set period size to 32 frames
    mic_array[i].frames = 32;
    snd_pcm_hw_params_set_period_size_near(mic_array[i].pcm_handle, mic_array[i].params, &mic_array[i].frames, &mic_array[i].dir);

    // Write the parameters to the driver
    rc = snd_pcm_hw_params(mic_array[i].pcm_handle, mic_array[i].params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(rc));
        return -1;
    }

    // Use a buffer large enough to hold one period
    snd_pcm_hw_params_get_period_size(mic_array[i].params, &mic_array[i].frames, &mic_array[i].dir);
    mic_array[i].buffer_size = mic_array[i].frames * CHANNELS * 2; // 2 bytes per sample (16-bit)
    mic_array[i].buffer = (char *) malloc((mic_array[i].buffer_size));
    printf("Mic configured\n");
    

    }

    return 1;
}

int close_capture_devices(mic_array_t *mic_array){
    for(int i = 0; i < NUM_MICS; i++){
        // Clean up
    printf("clean up time\n");
    free(mic_array[i].buffer);
    fclose(mic_array[i].recFile);
    snd_pcm_drain(mic_array[i].pcm_handle);
    snd_pcm_close(mic_array[i].pcm_handle);
    snd_pcm_hw_params_free(mic_array[i].params);
    
    }

    return 1;
}

int open_file_and_record(mic_array_t *mic_array){
    // Open the output file
    for(int i = 0; i < NUM_MICS; i++){
    mic_array[i].recFile = fopen("capture.wav", "wb");
    if (mic_array[i].recFile == NULL) {
        fprintf(stderr, "Unable to open output file\n");
        return -1;
    }
    int rc;
    // Capture data for the specified duration
    int loops = (CAPTURE_DURATION * SAMPLE_RATE) / (mic_array[i].frames);
    
    printf("About to record Loops: %d \n", loops);
    printf("Frames = %lu, Buffer Size:%d\n",(mic_array[i].frames), (mic_array[i].buffer_size));
    while (loops > 0) {
        loops--;
        rc = snd_pcm_readi(mic_array[i].pcm_handle, mic_array[i].buffer, (mic_array[i].frames));
        //printf("RC: %d\n",rc);
        if (rc == -EPIPE) {
            // EPIPE means overrun
            fprintf(stderr, "Overrun occurred\n");
            snd_pcm_prepare(mic_array[i].pcm_handle);
        } else if (rc < 0) {
            fprintf(stderr, "Error from read: %s\n", snd_strerror(rc));
        } else if (rc != (int)(mic_array[i].frames)) {
            fprintf(stderr, "Short read, read %d frames\n", rc);
        }
        fwrite(mic_array[i].buffer, 1, mic_array[i].buffer_size, mic_array[i].recFile);
    }
    printf("Finished recording \n");
    }
    return 1;
}