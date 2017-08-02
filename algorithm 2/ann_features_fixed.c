/*
 * Speed Detection with Artificial Neural Network on MSP430
 * 	Author: Frank Lehmke
 *
 */


#include <msp430.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
//#include <in430.h>
#include "ann_features_fixed.h"


#ifndef WINDOW_SIZE
#define WINDOW_SIZE 150
#endif

#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE WINDOW_SIZE
#endif

#define INVALID_VALUE SHRT_MIN //-32767 can not be reached with 6g Acc.
#define INIT_VALUE 0

#ifndef FRACTIONALBITS
#define FRACTIONALBITS 16
#endif

#ifndef AXISNUMBER
#define AXISNUMBER 3
#endif

// suppress warnings about optimization settings (via #pragma optimize)
#pragma diag_suppress = Go003

// Ringbuffer structure
static struct
{
    uint16_t pos; // points to the last added data point
    uint16_t oldPos; // points to the last added data point

    int16_t y[RINGBUFFER_SIZE];
#if AXISNUMBER == 3
    int16_t x[RINGBUFFER_SIZE];
    int16_t z[RINGBUFFER_SIZE];
#endif
} ringbuf;

static uint16_t windowSizeFixed = 0;

static struct
{
    int32_t mean[AXISNUMBER];
    uint32_t variance[AXISNUMBER];
    uint16_t amplitude[AXISNUMBER];
    uint64_t rmsSquared[AXISNUMBER];
} features;


/**
* Converts the window size into a Q0.f fixed-point representation.
* Default format: Q0.16
*/
void convertWindowSizeFixed(uint16_t windowSize)
{
    float temp;
    // first step: get fractional value as float
    temp = 1.0 / windowSize;

    // second step: convert float value to fixed-point value
    temp = temp * (1UL << FRACTIONALBITS);
    windowSizeFixed = 1 + (uint16_t)temp;
}

/**
* Set buffer pointer to inital value and clear buffer contents
*/
void initRingBuffer(void)
{
    uint16_t i;

    // Set pointer for buffer
    ringbuf.pos = 0;
    ringbuf.oldPos = RINGBUFFER_SIZE - WINDOW_SIZE;

    // Clear content
    for (i = 0; i < RINGBUFFER_SIZE; ++i)
    {
        ringbuf.y[i] = INIT_VALUE;
#if AXISNUMBER == 3
        ringbuf.x[i] = INIT_VALUE;
        ringbuf.z[i] = INIT_VALUE;
#endif
    }
}

/**
 * Insert new Data into ring buffer, overwrite old content
 */
void addValue(int16_t* value)
{
#if AXISNUMBER == 3
    ringbuf.x[ringbuf.pos] = value[0];
    ringbuf.y[ringbuf.pos] = value[1];
    ringbuf.z[ringbuf.pos] = value[2];
#elif AXISNUMBER == 1
    ringbuf.y[ringbuf.pos] = value[0];
#endif
}

/**
* A very efficient implementation of a square root routine from Crenshaw.
*
* http://embedded.com/electronics-blogs/programmer-s-toolbox/4219659/Integer-Square-Roots
*/
static uint16_t efficientSqrt(uint32_t a)
{
    unsigned long rem = 0;
    unsigned long root = 0;

    for (int i = 0; i < 16; i++)
    {
        root <<= 1;
        rem = ((rem << 2) + (a >> 30));
        a <<= 2;
        root++;
        if (root <= rem)
        {
            rem -= root;
            root++;
        }
        else
        {
            root--;
        }
    }
    return (unsigned short)(root >> 1);
}

// von Bernd
static uint16_t rootBernd(uint32_t x)
{
    unsigned long a, b;
    b = x;
    a = x = 0x3f;
    x = b / x;
    a = x = (x + a) >> 1;
    x = b / x;
    a = x = (x + a) >> 1;
    x = b / x;
    x = (x + a) >> 1;
    return x;
}

/*
 * Calculates the mean of the array.
 */
void calculateMean(int16_t* data, int32_t* result)
{
    uint16_t i, axes;

    __disable_interrupt();
    __no_operation();

    RESLO = 0; // clear HWMPY output registers
    RESHI = 0;
    MACS = windowSizeFixed; // Load Operand 1 (window size = 1/N)

    // cycle every data point in buffer
    for (int i = 0; i < RINGBUFFER_SIZE; i++)
    {
        OP2 = data[i]; // Load Operand 2 (Acceleration data)
    }

    // features.mean[axes] = *(int32_t *)&RESLO; // Get result from HWMPY
    *result = *(int32_t*)&RESLO; // Get result from HWMPY
    __enable_interrupt();
}

/**
* Determine variance of data by using results from RMS and mean. Function must
* be called AFTER the features RMS and mean have been calculated!
*/
void calculateVariance(uint64_t* rms, int32_t* mean, uint32_t* result)
{
    uint64_t variance = 0;
    uint64_t meanSquared = 0;
    // rms comes in Q32.16, so shifting results in (pseudo) Q32.32
    variance = *rms << 16;

    __disable_interrupt();
    __no_operation();
    // substract squared mean value (Q32.32) from rms (Q32.32)
    meanSquared = multiply32ASM(*mean, *mean);
    __enable_interrupt();

    if (variance >= meanSquared)
    {
        variance -= meanSquared;
    }
    else
    {
        variance = 0;
    }

    // result is 64 bit long, so discard frational bits yielding in Q32.0
    *result = variance >> 32;
}

/*
 * Calculates the running windowed unbiased sample variance for new data samples based
 * on own transformation of the original unbiased sample variance formula.
 *
 * Function will calculate the variance with the original formula in the first run, so it
 * requires a filled data array of size WINDOWSIZE. The mean and variance of the given data is
 * then calculated and saved for future function calls. The next time the function is
 * called only new data has to be passed.
 *
 * TODO: now, assuming an online version with length of newData=1 after first run.
 */
/*
#pragma optimize=speed
void calculateVarianceRunning(enum Axis accAxis) {
        //static bool firstRun = true;
        static float oldMean[3] = {0, 0, 0 };
        static float oldVariance[3] = {0, 0, 0 };
        float newMean;

        int32_t newData, oldData;

        if (accAxis == xAxis) {
          newData = (int32_t) RingBufferX[RingBufferXPos];
          oldData = (int32_t) RingBufferX[RingBufferXOldPos];
        } else if (accAxis == yAxis) {
          newData = (int32_t) RingBufferY[RingBufferYPos];
          oldData = (int32_t) RingBufferY[RingBufferYOldPos];
        } else if (accAxis == zAxis) {
          newData = (int32_t) RingBufferZ[RingBufferZPos];
          oldData = (int32_t) RingBufferZ[RingBufferZOldPos];
        }
        // calculate updated mean after first run
        // TODO: detect small changes in Mean calculation
        newMean = oldMean[accAxis] + (float)(newData - oldData)/WINDOWSIZE;
        // calculate updated unbiased variance
        oldVariance[accAxis] += (((newData-newMean) * (newData-newMean)) -
((oldData-oldMean[accAxis]) * (oldData-oldMean[accAxis]))) / (WINDOWSIZE-1);
        oldMean[accAxis] = newMean;

        variance[accAxis] = oldVariance[accAxis];
        //return oldVariance;
        //addValue( newData );
}
*/

/*
 * Returns actual Variance of the data
 */
uint32_t getVariance(enum Axis accAxis)
{
    return (uint32_t)features.variance[accAxis];
}


/*
 * Updates the amplitude with the new Data point and saves the greates amplitude
 * of the actual data set
 */
void calculateAmplitude(int16_t* data, uint16_t* result)
{
    int16_t min = 0, max = 0;
    uint16_t i;
    int16_t currentData;

    max = data[0];
    min = max;
    for (i = 1; i < RINGBUFFER_SIZE; i++)
    {
        currentData = data[i];
        if (currentData > max)
            max = currentData;
        else if (currentData < min)
            min = currentData;
    }

    *result = (uint16_t)(max - min);
    // features.amplitude[accAxis] = (uint16_t) (max - min);
}

uint16_t getAmplitude(enum Axis accAxis)
{
    return features.amplitude[accAxis];
}


#pragma optimize = speed
void calculateRMS(int16_t* data, uint64_t* result)
{
    uint32_t valueSquared = 0;
    uint64_t rmsSquared = 0;

    __disable_interrupt();
    __no_operation();


    // cycle every data point in buffer
    for (int i = 0; i < RINGBUFFER_SIZE; i++)
    {
        // int16_t mask = data[i] >> sizeof(int16_t) * CHAR_BIT - 1;
        // uint16_t r = (data[i] + mask) ^ mask;

        MPYS = data[i]; // Load Operand 1
        OP2 = data[i]; // Load Operand 2 (just square the input)

        valueSquared = *(uint32_t*)&RESLO;
        rmsSquared += multiply32_16ASM(valueSquared, windowSizeFixed);
    }

    *result = rmsSquared;
    __enable_interrupt();
}

/*
 * Calculates the running root mean square for new data samples based
 * on own transformation of the original root mean square formula.
 *
 * Function will calculate the rms value with the original formula in the first run, so it
 * requires a filled data array of size WINDOWSIZE. The rms value of the given data is
 * then calculated and saved as rms^2 for future function calls. The next time the function is
 * called only new and old data has to be passed.
 *
 * TODO: now, assuming an online version with length of newData=1 after first run.
 */
/*
void calculateRMSRunning(enum Axis accAxis) {
        //static bool firstRun = true;
        //static unsigned int oldRMSsquared[3] = {0, 0, 0};
        static float oldRMSsquared[3] = {0, 0, 0};
        //static short oldData = 0; // could also be a pointer to an array of old data

        int32_t newData, oldData;

        if (accAxis == xAxis) {
          newData = (int32_t) RingBufferX[RingBufferXPos];
          oldData = (int32_t) RingBufferX[RingBufferXOldPos];
        } else if (accAxis == yAxis) {
          newData = (int32_t) RingBufferY[RingBufferYPos];
          oldData = (int32_t) RingBufferY[RingBufferYOldPos];
        } else if (accAxis == zAxis) {
          newData = (int32_t) RingBufferZ[RingBufferZPos];
          oldData = (int32_t) RingBufferZ[RingBufferZOldPos];
        }

        // calculate updated rms
        //oldRMSsquared += ((newData[0] * newData[0]) - (oldData[0] * oldData[0])) *
0.00333;//WINDOWSIZE;

        oldRMSsquared[accAxis] += ((newData * newData) - (oldData * oldData)) / WINDOWSIZE;//;


        rmsSquared[accAxis] = (uint32_t) oldRMSsquared[accAxis];
        //return sqrt(oldRMSsquared);
        //addValue( newData );
}
*/

uint16_t getRMS(enum Axis accAxis)
{
    // return (uint16_t) sqrt(rmsSquared[accAxis]);
    return efficientSqrt(features.rmsSquared[accAxis] >> 16);
    // return rootBernd(rmsSquared[accAxis]);
}

/**
*
*/
void updateBufferPointer()
{

    ringbuf.pos++;
    ringbuf.oldPos++;

    if (ringbuf.pos >= RINGBUFFER_SIZE)
        ringbuf.pos = 0;
    if (ringbuf.oldPos >= RINGBUFFER_SIZE)
        ringbuf.oldPos = 0;
}

void generateFeatures()
{
#if AXISNUMBER > 1
    for (uint8_t i = 0; i < AXISNUMBER; i++)
    {
#endif
        int16_t* dataPointer;

#if AXISNUMBER == 3
        switch (i)
        {
            case 0:
                dataPointer = ringbuf.x;
                break;
            case 1:
                dataPointer = ringbuf.y;
                break;
            case 2:
                dataPointer = ringbuf.z;
                break;
        }
#elif AXISNUMBER == 1
    dataPointer = ringbuf.y;
#endif

        // addValue( newData );
        calculateMean(dataPointer, &features.mean[i]);
        calculateRMS(dataPointer, &features.rmsSquared[i]);

        calculateVariance(&features.rmsSquared[i], &features.mean[i], &features.variance[i]);
        calculateAmplitude(dataPointer, &features.amplitude[i]);

        updateBufferPointer();
#if AXISNUMBER > 1
    }
#endif
}
