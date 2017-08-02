#include <stdint.h>
#include <limits.h>

#ifndef RINGBUFFER_SIZE
#define RINGBUFFER_SIZE 300
#endif
#define INVALID_VALUE SHRT_MIN //-32767 can not be reached with 6g Acc.
#define LAST_MEAN_VALUES 3

static int16_t RingBuffer[RINGBUFFER_SIZE];
static uint16_t RingBufferCurrent; // points to the last added data point

void InitRingBuffer(void)
{
    uint16_t i;

    // RingBufferCurrent = &RingBuffer[0];
    RingBufferCurrent = 0;
    for (i = 0; i < RINGBUFFER_SIZE; ++i)
        RingBuffer[i] = INVALID_VALUE;
}

/**
 * Insert new Data into ring buffer
 */
void addValue(int16_t Value)
{
    RingBuffer[RingBufferCurrent++] = Value;
    if (RingBufferCurrent >= RINGBUFFER_SIZE)
        RingBufferCurrent = 0;
}

int MeanValue(void)
{
    uint16_t i;
    unsigned char nrValues = 0;
    long Summ = 0;

    for (i = 0; i < RINGBUFFER_SIZE; ++i)
    {
        if (RingBuffer[i] != INVALID_VALUE)
        {
            Summ += RingBuffer[i];
            nrValues++;
        }
    }

    if (nrValues == 0)

        return Summ / nrValues;
}

int MeanValueOfLast(void)
{
    long Summ = 0;
    uint16_t i = RingBufferCurrent;
    unsigned char nrValues = 0;

    do
    {
        if (i == 0)
            i = RINGBUFFER_SIZE - 1;
        else
            --i;

        if (RingBuffer[i] != INVALID_VALUE)
        {
            Summ += RingBuffer[i];
            nrValues++;
        }
    } while (nrValues != LAST_MEAN_VALUES && i != RingBufferCurrent);

    if (i == RingBufferCurrent)
        return INVALID_VALUE;

    return Summ / nrValues;
}