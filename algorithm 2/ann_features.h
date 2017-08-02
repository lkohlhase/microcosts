/*
 * Speed Detection with Artificial Neural Network on MSP430
 * 	Author: Frank Lehmke
 *
 */

#ifndef ANN_FEATURES_H_
#define ANN_FEATURES_H_

#include <stdint.h>

// Support both C99 und GNU compiler
#ifndef INLINE
# if __GNUC__ && !__GNUC_STDC_INLINE__
#  define INLINE inline
# else
#  define INLINE extern inline
# endif
#endif

#define WINDOWSIZE 150

enum Axis{
	xAxis = 0, yAxis = 1, zAxis = 2
};

void initRingBuffer( void );
void calculateVariance(enum Axis accAxis);
void calculateVarianceRunning(enum Axis accAxis);
void addValue( int16_t value, enum Axis accAxis );
void updateBufferPointer(enum Axis accAxis);

uint32_t getVariance(enum Axis accAxis);

void calculateAmplitude(enum Axis);
uint16_t getAmplitude(enum Axis accAxis);
void calculateRMS(int16_t * data, enum Axis accAxis);
void calculateRMSRunning(enum Axis accAxis);
uint16_t getRMS(enum Axis accAxis);

void generateFeatures(int16_t newData, enum Axis accAxis);

#endif /* ANN_FEATURES_H_ */
