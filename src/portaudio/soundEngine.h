//
// Created by Zoe Bell on 5/1/24.
//
//#ifndef GRAPHICS_SOUNDENGINE_H
//#define GRAPHICS_SOUNDENGINE_H

#include "portaudio.h"
#include <math.h>
#include <stdio.h>

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)
#define FREQUENCY 220

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)

typedef struct {
    float sine[TABLE_SIZE];
    int phase;
}
paTestData;

class SoundEngine {
public:
    bool turnOff;

    void run() {
        PaStreamParameters outputParameters;
        int i;
        double t;
        turnOff = false;

        static unsigned long n = 0;
        for(i = 0; i < TABLE_SIZE; i++, n++) {
            t = (double)i/(double)SAMPLE_RATE;
            data.sine[i] = 0;
            data.sine[i] = 0.3 * sin(2 * M_PI * FREQUENCY * t);
            data.sine[i] *= 1.0/2;
            data.sine[i] += 0.5 * sin(2 * M_PI * (FREQUENCY + 110) * t);
            data.sine[i] *= 2.0/3;
            data.sine[i] += (1.0/3) * sin(2 * M_PI * (FREQUENCY + 60) * t);
            data.sine[i] *= 3.0/4;
            data.sine[i] += (1.0/4) * sin(2 * M_PI * (FREQUENCY + 160) * t);
        }
        data.phase = 0;

        error = Pa_Initialize();
        if(error != paNoError) {
            cout << Pa_GetErrorText(error) << endl;
        }

        outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */

        outputParameters.channelCount = 2;       /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        error = Pa_OpenStream(
                &stream,
                NULL, /* no input */
                &outputParameters,
                SAMPLE_RATE,
                FRAMES_PER_BUFFER,
                paClipOff,      /*paNoFlag we won't output out of range samples so don't bother clipping them */
                patestCallback,
                &data );

        if(error != paNoError) {
            cout << Pa_GetErrorText(error) << endl;
        }

    }

    static int patestCallback( const void *inputBuffer, void *outputBuffer,unsigned long framesPerBuffer,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *userData ) {
        paTestData *callData = (paTestData*)userData;
        float *out = (float*)outputBuffer;
        float sample;
        unsigned long i;

        (void) timeInfo; /* Prevent unused variable warnings. */
        (void) statusFlags;
        (void) inputBuffer;

        for( i=0; i<framesPerBuffer; i++ ) {
            sample = callData->sine[callData->phase++];
            *out++ = sample;  /* left */
            *out++ = sample;  /* right */
            if( callData->phase >= TABLE_SIZE ) callData->phase -= TABLE_SIZE;
        }

        return paContinue;
    }

    void makeSine(float frequency) {
        if(!pressed) {
            for(int i=0; i<TABLE_SIZE; i++) {
                data.sine[i] += 0.3*sin(2 * M_PI * frequency * ((double)i/(double)SAMPLE_RATE));
            }
            pressed = true;
            error = Pa_StartStream( stream );
        }
    }
    void stopSine(float frequency) {
        error = Pa_StopStream( stream );
        for(int i=0; i<TABLE_SIZE; i++) {
            data.sine[i] -= 0.3*sin(2 * M_PI * frequency * ((double)i/(double)SAMPLE_RATE));
        }
        pressed = false;
    }

private:
    paTestData data;
    PaStream *stream;
    PaError error;
    bool pressed;
};

//#endif //GRAPHICS_SOUNDENGINE_H
