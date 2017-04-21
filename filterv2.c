/*
 * filter.c
 *
 *  Created on: 15.03.1012
 *      Author: Bernd
 */

#include <msp430FG4619.h>
#include <legacymsp430.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "filter.h"


//------- Notch test-------
#define DCgain 1
//------- Notch test-------

float notch_b[3] = {1.0 , 1.618 , 1.0};
float notch_a[3] = {1.0 , 1.5164 , 0.8783};

//float notch_fiir(float NewSample);

void fil_init()
{
  fil_lpfilt(0,1);
  fil_hpfilt(0,1);
  fil_deriv1(0,1);
  fil_mvwint(0,1);
}

/*************************************************************************
*  lpfilt() implements the digital filter represented by the difference
*  equation:
*
* 	y[n] = 2*y[n-1] - y[n-2] + x[n] - 2*x[n-5] + x[n-10]
*
*	Note that the filter delay is five samples.
*
**************************************************************************/
   
signed int fil_lpfilt( signed int datum ,int init)
	{
          
        static long y1 = 0, y2 = 0 ;
	static int data[LPBUFFER_LGTH], ptr = 0 ;
	long y0 ;
	int output, halfPtr ;
	if(init)
		{
		for(ptr = 0; ptr < LPBUFFER_LGTH; ++ptr)
			data[ptr] = 0 ;
		y1 = y2 = 0 ;
		ptr = 0 ;
		}
	halfPtr = ptr-(LPBUFFER_LGTH/2) ;	// Use halfPtr to index
	if(halfPtr < 0)							// to x[n-5].
		halfPtr += LPBUFFER_LGTH ;
	y0 = (y1 << 1) - y2 + datum - (data[halfPtr] << 1) + data[ptr] ;
	y2 = y1;
	y1 = y0;
	output = y0 / ((LPBUFFER_LGTH*LPBUFFER_LGTH)/4);
	data[ptr] = datum ;			// Stick most recent sample into
	if(++ptr == LPBUFFER_LGTH)	// the circular buffer and update
		ptr = 0 ;					// the buffer pointer.
	return(output) ;
	}

/******************************************************************************
*  hpfilt() implements the high pass filter represented by the following
*  difference equation:
*
*	y[n] = y[n-1] + x[n] - x[n-32]
*	z[n] = x[n-16] - y[n] ;
*
*  Note that the filter delay is 15.5 samples
******************************************************************************/
signed int fil_hpfilt( signed int datum, int init )
	{
          
	static long y=0 ;
	static int data[HPBUFFER_LGTH], ptr = 0 ;
	int z, halfPtr ;
    
	if(init)
		{
		for(ptr = 0; ptr < HPBUFFER_LGTH; ++ptr)
			data[ptr] = 0 ;
		ptr = 0 ;
		y = 0 ;
		return(0) ;
		}
       
	y += datum - data[ptr];
	halfPtr = ptr-(HPBUFFER_LGTH/2) ;
	if(halfPtr < 0)
		halfPtr += HPBUFFER_LGTH ;
	z = data[halfPtr] - (y / HPBUFFER_LGTH);

	data[ptr] = datum ;
	if(++ptr == HPBUFFER_LGTH)
		ptr = 0 ;

	return( z );
	}


/*****************************************************************************
*  deriv1 and deriv2 implement derivative approximations represented by
*  the difference equation:
*
*	y[n] = 2*x[n] + x[n-1] - x[n-3] - 2*x[n-4]
*
*  The filter has a delay of 2.
*****************************************************************************/


signed int fil_deriv1( signed int x0, int init )
	{
    static signed int x1, x2, x3, x4 ;
	
	signed int output;
	if(init)
		x1 = x2 = x3 = x4 = 0 ;
    //printf("Eingabe Derivative-Filter: %i  \n", x0);
	output = x1-x3 ;
	if(output < 0) output = (output>>1) | 0x8000 ;	// Compensate for shift bug.
	else output >>= 1 ;

	output += (x0-x4) ;
	if(output < 0) output = (output>>1) | 0x8000 ;
	else output >>= 1 ;
	
	
	x4 = x3 ;
	x3 = x2 ;
	x2 = x1 ;
	x1 = x0 ;
	return(output);
	}


/*****************************************************************************
* mvwint() implements a moving window integrator, averaging
* the signal values over the last 16
******************************************************************************/

int fil_mvwint(signed int datum, int init)
	{
    static unsigned int sum = 0 ;
    static unsigned int d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15 ;

	if(init)
		{
		d0=d1=d2=d3=d4=d5=d6=d7=d8=d9=d10=d11=d12=d13=d14=d15=0 ;
		sum = 0 ;
		}
    //printf("Eingabe MVW-Filter: %i  \n", datum);
	sum -= d15 ;

	d15=d14 ;
	d14=d13 ;
	d13=d12 ;
	d12=d11 ;
	d11=d10 ;
	d10=d9 ;
	d9=d8 ;
	d8=d7 ;
	d7=d6 ;
	d6=d5 ;
	d5=d4 ;
	d4=d3 ;
	d3=d2 ;
	d2=d1 ;
	d1=d0 ;
	if(datum >= 0x0400) d0 = 0x03ff ;
	else d0 = (datum>>2) ;
	sum += d0 ;
	return(sum>>2) ;
	}





#define NFCoef 4
/*
float AFCoef[NFCoef+1] = {
        0.64289373826848084000,
        -0.58867357096268036000,
        1.42054402825081420000,
        -0.58867357096268036000,
        0.64289373826848084000
    };

    float BFCoef[NFCoef+1] = {
        1.00000000000000000000,
        -0.72974104531597572000,
        1.32260313009942050000,
        -0.44786188546482963000,
        0.38431634854829128000
    };
*/
/*
float AFCoef[NFCoef+1] = {
        0.642,
        -0.588,
        1.420,
        -0.588,
        0.642
    };

    float BFCoef[NFCoef+1] = {
        1.000,
        -0.729,
        1.322,
        -0.447,
        0.384
    };
    
float notch_fiir(float NewSample) {
    

    static float y[NFCoef+1]; //output samples
    static float x[NFCoef+1]; //input samples
    int n;

    //shift the old samples
    for(n=NFCoef; n>0; n--) {
       x[n] = x[n-1];
       y[n] = y[n-1];
    }

    //Calculate the new output
    x[0] = NewSample;
    y[0] = AFCoef[0] * x[0];
    for(n=1; n<=NFCoef; n++)
        y[0] += AFCoef[n] * x[n] - BFCoef[n] * y[n];
    
    return y[0];
}*/

