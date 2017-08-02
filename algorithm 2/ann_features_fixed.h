/*
 * Speed Detection with Artificial Neural Network on MSP430
 * 	Author: Frank Lehmke
 *
 */

#ifndef ANN_FEATURES_H_
#define ANN_FEATURES_H_

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
void convertWindowSizeFixed( uint16_t fractionSize );
void calculateVariance(uint64_t * rms, int32_t * mean, uint32_t * result);
void calculateVarianceRunning(enum Axis accAxis);

uint32_t getVariance(enum Axis accAxis);

void calculateAmplitude(int16_t * data, uint16_t * result);
uint16_t getAmplitude(enum Axis accAxis);
void calculateRMS(int16_t * data, uint64_t * result);
void calculateRMSRunning(enum Axis accAxis);
uint16_t getRMS(enum Axis accAxis);

void generateFeatures();

extern uint64_t multiply32ASM(uint32_t operand1, uint32_t operand2); /* Function Prototype for asm function */
extern uint64_t multiply32_16ASM(uint32_t operand1, uint16_t operand2); /* Function Prototype for asm function */
#endif /* ANN_FEATURES_H_ */
