#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <libusb-1.0/libusb.h>
#include <pthread.h>
#include "mic_array.h"
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <glib.h>
#include <gst/gst.h>
//#define PCM_DEVICE "plughw:1,0"
//#define CAPTURE_DURATION 1
//#define SAMPLE_RATE 44100    // Sampling rate in Hz
//#define PCM_FORMAT SND_PCM_FORMAT_S16_LE  // PCM format (16-bit little-endian)
//#define CAPTURE_DURATION 1

static GMainLoop *loop;
static GstElement *textoverlay;
static int gain = 1;
static int doa = 0;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            //g_print("Some Message Recevied...\n");
            break;
    }

    return TRUE;
}

void *update_text(void *data) {
    
    gchar text[100];

    while (1) {
        // Create a dynamic text
        snprintf(text, sizeof(text), "Gain: %d, DOA: %d", gain, doa);
        
        // Update the textoverlay element
        g_object_set(G_OBJECT(textoverlay), "text", text, NULL);
        
        // Sleep for 1 second before updating again
        sleep(1);
    }

    return NULL;
}

//set to noncanonical mode to capture key presses
void set_noncanonical_mode(){
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
}

void restore_terminal_mode(){

    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

char get_key_press(){
    char ch=0;
    if(read(STDIN_FILENO, &ch, 1) == -1){
        return 0;
    }
    return ch;
}


volatile sig_atomic_t stop = 0;

volatile int running = 1;
volatile int thread_running = 1;

void handle_sigint(int sig){
    printf("Caught Interrupt signal %d, cleaning up...\n",sig);
    g_main_loop_quit(loop);
    //running = 0;
    stop = 1;
}

void *audio_thread_main(void *arg){
        char key_press;

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
        myArray[i]->delay = 0;
    }   

    if(open_and_configure_capture_devices(myArray) < 0){
        printf("Error open and configure\n");
        return (void *)-1;
    }

    setup_output_pcm(&playback_handle);
    
    mixed_buffer = malloc(FRAMES * PLAYBACK_FRAMES);
    gain = 1;
    doa = 0;
    while(running && !stop){
                key_press = get_key_press();
                if (key_press == 'q')
                {
                    printf("Terminating program...\n");
                    running = 0;
                }
                else if (key_press == 'w')
                {
                    gain += 1;
                    printf("Increasing gain\n");
                    printf("Gain : %d\n", gain);
                }
                else if (key_press == 's')
                {
                    gain -= 1;
                    if (gain < 0)
                    {
                        gain = 0;
                    }
                    printf("Decreasing gain\n");
                    printf("Gain : %d\n", gain);
                }
                else if(key_press == 'd'){
                    doa += 45;
                    if(doa > 90){
                        doa = 90;
                    }    
                    printf("DOA : %d\n",doa);
                    set_mic_delays(myArray, doa);
                }
                else if(key_press == 'a'){
                    doa -= 45;
                    if(doa < -90){
                        doa = -90;
                    }
                    printf("DOA : %d\n",doa);
                    set_mic_delays(myArray,doa);
                }
                else
                {
                    //printf("Key pressed is %c\n",key_press);
                }

        memset(mixed_buffer,0,FRAMES*PLAYBACK_FRAMES);
        capture_audio(myArray, mixed_buffer, PLAYBACK_FRAMES, gain);

        play_audio(mixed_buffer, FRAMES, playback_handle);

        if(stop){
            printf("Caught SIGINT, cleaning up now...\n");
            break;
        }
    }

    
    close_capture_devices(myArray);
    for(int i = 0 ; i < MAX_DEVICES; i++){
        free(myArray[i]);
    }
    snd_pcm_close(playback_handle);
    thread_running = 0;
    g_main_loop_quit(loop);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, handle_sigint);

    pthread_t audio_thread, text_thread;

    set_noncanonical_mode();

    GstElement *pipeline, *source, *convert, *sink;
    GstBus *bus;
    GstCaps *caps;
  

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the main loop
    loop = g_main_loop_new(NULL, FALSE);

    // Create the elements
    source = gst_element_factory_make("v4l2src", "source");
    convert = gst_element_factory_make("videoconvert", "convert");
    textoverlay = gst_element_factory_make("textoverlay", "textoverlay");
    sink = gst_element_factory_make("autovideosink", "sink");

    if (!source || !convert || !textoverlay || !sink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

        // Define the caps for the source (v4l2src)
    caps = gst_caps_new_simple("video/x-raw",
                               "width", G_TYPE_INT, 640,
                               "height", G_TYPE_INT, 480,
                               "framerate", GST_TYPE_FRACTION, 30, 1,
                               "format", G_TYPE_STRING, "YUY2",
                               "interlace-mode", G_TYPE_STRING, "progressive",
                               "colorimetry", G_TYPE_STRING, "2:4:16:1",
                               NULL);

        // Set the custom text overlay properties
    g_object_set(G_OBJECT(textoverlay),
                 "text", "Your Custom Text",   // The text to overlay
                 "valignment", 0,              // Align the text to the top (0 = top, 1 = center, 2 = bottom)
                 "halignment", 0,              // Align the text to the left (0 = left, 1 = center, 2 = right)
                 "font-desc", "Sans, 24",      // Font description
                 NULL);

// Create the empty pipeline
    pipeline = gst_pipeline_new("video-pipeline");

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, convert, textoverlay, sink, NULL);

    // Link the source with caps to videoconvert
    if (!gst_element_link_filtered(source, convert, caps)) {
        g_printerr("Source and convert could not be linked with caps. Exiting.\n");
        gst_caps_unref(caps);
        gst_object_unref(pipeline);
        return -1;
    }

    // Link the rest of the pipeline
    if (!gst_element_link_many(convert, textoverlay, sink, NULL)) {
        g_printerr("Convert, textoverlay, and sink could not be linked. Exiting.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Free caps after usage
    gst_caps_unref(caps);

    // Set the pipeline to "playing" state
    gst_element_set_state(pipeline, GST_STATE_PLAYING);


    pthread_create(&audio_thread,NULL,audio_thread_main,NULL);

    // Set up a message handler on the bus
    bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    pthread_create(&text_thread, NULL, update_text, NULL);
    // Run the main loop
    g_main_loop_run(loop);

    pthread_join(audio_thread,NULL);

    // Clean up after exiting the loop
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    //pthread_create(&audio_thread,NULL,audio_thread_main,NULL);
    //pthread_join(audio_thread,NULL);
    restore_terminal_mode();
    return 0;

 
}
