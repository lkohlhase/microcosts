/*
 * Speed Detection with Artificial Neural Network on MSP430
 * 	Author: Frank Lehmke
 *
 */


#include <math.h>
//#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "ann_features.h"


#ifndef WINDOW_SIZE
#define WINDOW_SIZE 150
#endif

#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE WINDOW_SIZE + 1
#endif

#define INVALID_VALUE SHRT_MIN //-32767 can not be reached with 6g Acc.
#define INIT_VALUE 0


static int16_t RingBufferX[RINGBUFFER_SIZE];
static uint16_t RingBufferXPos; // points to the last added data point
static uint16_t RingBufferXOldPos; // points to the last added data point

static int16_t RingBufferY[RINGBUFFER_SIZE];
static uint16_t RingBufferYPos; // points to the last added data point
static uint16_t RingBufferYOldPos; // points to the last added data point

static int16_t RingBufferZ[RINGBUFFER_SIZE];
static uint16_t RingBufferZPos; // points to the last added data point
static uint16_t RingBufferZOldPos; // points to the last added data point


#ifdef USEINTEGER
static uint32_t variance[3];
static uint16_t amplitude[3];
static uint32_t rmsSquared[3];
#endif

//#ifdef USEFLOAT
static float variance[3];
static uint16_t amplitude[3];
static uint32_t rmsSquared[3];
//#endif

void initRingBuffer(void)
{
    uint16_t i;

    // RingBufferCurrent = &RingBuffer[0];
    RingBufferXPos = 0;
    RingBufferYPos = 0;
    RingBufferZPos = 0;

    RingBufferXOldPos = RINGBUFFER_SIZE - WINDOW_SIZE;
    RingBufferYOldPos = RingBufferXOldPos;
    RingBufferZOldPos = RingBufferXOldPos;

    for (i = 0; i < RINGBUFFER_SIZE; ++i)
    {
        RingBufferX[i] = INIT_VALUE;
        RingBufferY[i] = INIT_VALUE;
        RingBufferZ[i] = INIT_VALUE;
    }
}

/**
 * Insert new Data into ring buffer
 * TODO: change this ugly thing
 */
void addValue(int16_t value, enum Axis accAxis)
{
    switch (accAxis)
    {
        case xAxis:
            RingBufferX[RingBufferXPos] = value;
            break;
        case yAxis:
            RingBufferY[RingBufferYPos] = value;
            break;
        case zAxis:
            RingBufferZ[RingBufferZPos] = value;
            break;
    }
}

static uint16_t efficientSqrt(uint32_t a)
{
    unsigned long rem = 0;
    unsigned long root = 0;
    int i;

    for (i = 0; i < 16; i++)
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
float calculateMean(int16_t* data)
{
    uint16_t i;
    float sum = 0;
    for (i = 0; i < WINDOWSIZE; i++)
        sum += data[i];

    return (float)sum / WINDOWSIZE;
}


void calculateVariance(enum Axis accAxis)
{
    uint16_t i;
    uint16_t pointer = RingBufferXPos;
    float sum = 0;
    // unsigned int expectationValue;
    float expectationValue;

    switch (accAxis)
    {
        case xAxis:
            pointer = RingBufferXPos;
            break;
        case yAxis:
            pointer = RingBufferYPos;
            break;
        case zAxis:
            pointer = RingBufferZPos;
            break;
    }

    // calculate expectation value E of data
    for (i = 0; i < WINDOWSIZE; i++)
    {
        switch (accAxis)
        {
            case xAxis:
                sum += RingBufferX[pointer];
                break;
            case yAxis:
                sum += RingBufferY[pointer];
                break;
            case zAxis:
                sum += RingBufferZ[pointer];
                break;
        }
        if (pointer == 0)
        {
            pointer = WINDOWSIZE;
        }
        else
        {
            pointer--;
        }
    }
    sum = (sum / (float)WINDOWSIZE);
    // expectationValue = (unsigned int) (sum + 0.5); //round result
    expectationValue = sum;

    sum = 0;
    switch (accAxis)
    {
        case xAxis:
            pointer = RingBufferXPos;
            break;
        case yAxis:
            pointer = RingBufferYPos;
            break;
        case zAxis:
            pointer = RingBufferZPos;
            break;
    }

    // calculate actual variance
    for (i = 0; i < WINDOWSIZE; i++)
    {
        switch (accAxis)
        {
            case xAxis:
                sum += (RingBufferX[pointer] - expectationValue)
                    * (RingBufferX[pointer] - expectationValue);
                break;
            case yAxis:
                sum += (RingBufferY[pointer] - expectationValue)
                    * (RingBufferY[pointer] - expectationValue);
                break;
            case zAxis:
                sum += (RingBufferZ[pointer] - expectationValue)
                    * (RingBufferZ[pointer] - expectationValue);
                break;
        }
        if (pointer == 0)
        {
            pointer = WINDOWSIZE;
        }
        else
        {
            pointer--;
        }
    }
    variance[accAxis] = sum / (float)(WINDOWSIZE - 1);
    // return (sum / (float)(WINDOWSIZE - 1)); // return unbiased variance
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
//#pragma optimize=speed
void calculateVarianceRunning(enum Axis accAxis)
{
    // static bool firstRun = true;
    static float oldMean[3] = { 0, 0, 0 };
    static float oldVariance[3] = { 0, 0, 0 };
    float newMean;

    int32_t newData, oldData;

    if (accAxis == xAxis)
    {
        newData = (int32_t)RingBufferX[RingBufferXPos];
        oldData = (int32_t)RingBufferX[RingBufferXOldPos];
    }
    else if (accAxis == yAxis)
    {
        newData = (int32_t)RingBufferY[RingBufferYPos];
        oldData = (int32_t)RingBufferY[RingBufferYOldPos];
    }
    else if (accAxis == zAxis)
    {
        newData = (int32_t)RingBufferZ[RingBufferZPos];
        oldData = (int32_t)RingBufferZ[RingBufferZOldPos];
    }
    // calculate updated mean after first run
    // TODO: detect small changes in Mean calculation
    newMean = oldMean[accAxis] + (float)(newData - oldData) / WINDOWSIZE;
    // calculate updated unbiased variance
    oldVariance[accAxis] += (((newData - newMean) * (newData - newMean))
                                - ((oldData - oldMean[accAxis]) * (oldData - oldMean[accAxis])))
        / (WINDOWSIZE - 1);
    oldMean[accAxis] = newMean;

    variance[accAxis] = oldVariance[accAxis];
    // return oldVariance;
    // addValue( newData );
}


/*
 * Returns actual Variance of the data
 */
uint32_t getVariance(enum Axis accAxis)
{
    return (uint32_t)variance[accAxis];
}


/*
 * Updates the amplitude with the new Data point and saves the greates amplitude
 * of the actual data set
 */
void calculateAmplitude(enum Axis accAxis)
{
    int16_t min = 0, max = 0;
    uint16_t i;
    uint16_t bias;

    int16_t currentData;

    if (accAxis == xAxis)
    {
        bias = RingBufferXOldPos;
        max = RingBufferX[bias];
        min = max;
        for (i = 0; i < WINDOWSIZE; i++)
        {
            if (bias >= RINGBUFFER_SIZE)
            {
                bias = 0;
            }
            currentData = RingBufferX[bias];
            if (currentData > max)
                max = currentData;
            else if (currentData < min)
                min = currentData;

            bias++;
        }
    }
    else if (accAxis == yAxis)
    {
        bias = RingBufferYOldPos;
        max = RingBufferY[bias];
        min = max;
        for (i = 0; i < WINDOWSIZE; i++)
        {
            if (bias >= RINGBUFFER_SIZE)
            {
                bias = 0;
            }
            currentData = RingBufferY[bias];
            if (currentData > max)
                max = currentData;
            else if (currentData < min)
                min = currentData;

            bias++;
        }
    }
    else if (accAxis == zAxis)
    {
        bias = RingBufferZOldPos;
        max = RingBufferZ[bias];
        min = max;
        for (i = 0; i < WINDOWSIZE; i++)
        {
            if (bias >= RINGBUFFER_SIZE)
            {
                bias = 0;
            }
            currentData = RingBufferZ[bias];
            if (currentData > max)
                max = currentData;
            else if (currentData < min)
                min = currentData;

            bias++;
        }
    }

    amplitude[accAxis] = (uint16_t)(max - min);
}

uint16_t getAmplitude(enum Axis accAxis)
{
    return amplitude[accAxis];
}

/*uint16_t multiply(int16_t data1) {
        return data1 * data1;
}*/

#define squaring(x) (x * x)
//#pragma optimize=speed
void calculateRMS(int16_t* data, enum Axis accAxis)
{
    uint16_t i;
    uint32_t rmsSqrt = 0;


    // calculate actual variance
    for (i = 0; i < WINDOWSIZE; i++)
    {
        rmsSqrt += (uint32_t)data[i] * data[i];
        // rmsSqrt += squaring(data[i]);
    }


    rmsSqrt = rmsSqrt / WINDOWSIZE;
    // rmsSqrt = rmsSqrt * 0.003333;

    rmsSquared[accAxis] = rmsSqrt;
    // return sqrt(rmsSquared);
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
void calculateRMSRunning(enum Axis accAxis)
{
    // static bool firstRun = true;
    // static unsigned int oldRMSsquared[3] = {0, 0, 0};
    static float oldRMSsquared[3] = { 0, 0, 0 };
    // static short oldData = 0; // could also be a pointer to an array of old data

    int32_t newData, oldData;

    if (accAxis == xAxis)
    {
        newData = (int32_t)RingBufferX[RingBufferXPos];
        oldData = (int32_t)RingBufferX[RingBufferXOldPos];
    }
    else if (accAxis == yAxis)
    {
        newData = (int32_t)RingBufferY[RingBufferYPos];
        oldData = (int32_t)RingBufferY[RingBufferYOldPos];
    }
    else if (accAxis == zAxis)
    {
        newData = (int32_t)RingBufferZ[RingBufferZPos];
        oldData = (int32_t)RingBufferZ[RingBufferZOldPos];
    }

    // calculate updated rms
    // oldRMSsquared += ((newData[0] * newData[0]) - (oldData[0] * oldData[0])) *
    // 0.00333;//WINDOWSIZE;

    oldRMSsquared[accAxis] += ((newData * newData) - (oldData * oldData)) / WINDOWSIZE; //;


    rmsSquared[accAxis] = (uint32_t)oldRMSsquared[accAxis];
}

uint16_t getRMS(enum Axis accAxis)
{
    // return (uint16_t) sqrt(rmsSquared[accAxis]);
    return efficientSqrt(rmsSquared[accAxis]);
    // return rootBernd(rmsSquared[accAxis]);
}

void updateBufferPointer(enum Axis accAxis)
{
    if (accAxis == xAxis)
    {
        RingBufferXPos++;
        RingBufferXOldPos++;
        if (RingBufferXPos >= RINGBUFFER_SIZE)
            RingBufferXPos = 0;
        if (RingBufferXOldPos >= RINGBUFFER_SIZE)
            RingBufferXOldPos = 0;
    }
    else if (accAxis == yAxis)
    {
        RingBufferYPos++;
        RingBufferYOldPos++;
        if (RingBufferYPos >= RINGBUFFER_SIZE)
            RingBufferYPos = 0;
        if (RingBufferYOldPos >= RINGBUFFER_SIZE)
            RingBufferYOldPos = 0;
    }
    else if (accAxis == zAxis)
    {
        RingBufferZPos++;
        RingBufferZOldPos++;
        if (RingBufferZPos >= RINGBUFFER_SIZE)
            RingBufferZPos = 0;
        if (RingBufferZOldPos >= RINGBUFFER_SIZE)
            RingBufferZOldPos = 0;
    }
}

void generateFeatures(int16_t newData, enum Axis accAxis)
{
    // addValue( newData, accAxis );
    // calculateVarianceRunning(accAxis);
    calculateVariance(accAxis);
    calculateRMSRunning(accAxis);
    calculateAmplitude(accAxis);

    // updateBufferPointer(accAxis);
}
