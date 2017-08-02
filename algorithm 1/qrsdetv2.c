/*
 * qrsdet.c
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
#include "qrsdet.h"
#include "filter.h"


extern int QRS;

// Global values for QRS detector.

int Q0 = 0, Q1 = 0, Q2 = 0, Q3 = 0, Q4 = 0, Q5 = 0, Q6 = 0, Q7 = 0;
int N0 = 0, N1 = 0, N2 = 0, N3 = 0, N4 = 0, N5 = 0, N6 = 0, N7 = 0;
int RR0 = 0, RR1 = 0, RR2 = 0, RR3 = 0, RR4 = 0, RR5 = 0, RR6 = 0, RR7 = 0;
int QSum = 0, NSum = 0, RRSum = 0;
int det_thresh, sbcount;

int tempQSum, tempNSum, tempRRSum;

int QN0 = 0, QN1 = 0;
int Reg0 = 0;

/******************************************************************************
*  PICQRSDet takes 12-bit ECG samples as input and returns the
*  detection delay when a QRS is detected.  Passing a nonzero value for init
*  resets the QRS detector.
******************************************************************************/
signed int qrsdet_PICQRSDet(signed int x, int init)
{
    // P6OUT ^= 0x80;
    static int tempPeak, initMax;
    static int preBlankCnt = 0, qpkcnt = 0, initBlank = 0;
    static int count, sbpeak, sbloc;
    int QrsDelay = 0;

    if (init)
    {
        fil_hpfilt(0, 1);
        fil_lpfilt(0, 1);
        fil_deriv1(0, 1);
        fil_mvwint(0, 1);
        qrsdet_peak(0, 1);
        qpkcnt = count = sbpeak = 0;
        QSum = NSum = 0;
        /*send_charCOM('I');
        send_charCOM(' ');     */
        RRSum = MS1000 << 3;
        RR0 = RR1 = RR2 = RR3 = RR4 = RR5 = RR6 = RR7 = MS1000;

        Q0 = Q1 = Q2 = Q3 = Q4 = Q5 = Q6 = Q7 = 0;
        N0 = N1 = N2 = N3 = N4 = N5 = N6 = N7 = 0;
        NSum = 0;

        return (0);
    }


    // x = lpfilt(x,0) ;

    // x = hpfilt(x,0) ;

    x = fil_deriv1(x, 0);

    if (x < 0)
        x = -x;
    // x = x*x;

    x = fil_mvwint(x, 0);


    x = qrsdet_peak(x, 0);


    // if(x != 0)
    //  printf("Peak: %i \n", x);
    // Hold any peak that is detected for 200 ms
    // in case a bigger one comes along.  There
    // can only be one QRS complex in any 200 ms window.

    if (!x && !preBlankCnt)
        x = 0;

    else if (!x && preBlankCnt) // If we have held onto a peak for
    { // 200 ms pass it on for evaluation.
        if (--preBlankCnt == 0)
            x = tempPeak;
        else
            x = 0;
    }

    else if (x && !preBlankCnt) // If there has been no peak for 200 ms
    { // save this one and start counting.
        tempPeak = x;
        preBlankCnt = MS200;
        x = 0;
    }

    else if (x) // If we were holding a peak, but
    { // this ones bigger, save it and
        if (x > tempPeak) // start counting to 200 ms again.
        {
            tempPeak = x;
            preBlankCnt = MS200;
            x = 0;
        }
        else if (--preBlankCnt == 0)
            x = tempPeak;
        else
            x = 0;
    }

    // Initialize the qrs peak buffer with the first eight
    // local maximum peaks detected.

    if (qpkcnt < 8)
    {
        ++count;
        if (x > 0)
            count = WINDOW_WIDTH;
        if (++initBlank == MS1000)
        {
            initBlank = 0;
            qrsdet_updateQ(initMax);
            initMax = 0;
            ++qpkcnt;
            if (qpkcnt == 8)
            {

                RRSum = MS1000 << 3;
                RR0 = RR1 = RR2 = RR3 = RR4 = RR5 = RR6 = RR7 = MS1000;

                sbcount = MS1500 + MS150;
            }
        }
        if (x > initMax)
            initMax = x;
    }

    else
    {
        ++count;

        // Check if peak is above detection threshold.

        if (x > det_thresh)
        {
            qrsdet_updateQ(x);

            // Update RR Interval esti<mate and search back limit

            qrsdet_updateRR(count - WINDOW_WIDTH);
            count = WINDOW_WIDTH;
            sbpeak = 0;
            QrsDelay = WINDOW_WIDTH + FILTER_DELAY;
        }

        // If a peak is below the detection threshold.

        else if (x != 0)
        {
            qrsdet_updateN(x);

            QN1 = QN0;
            QN0 = count;

            if ((x > sbpeak) && ((count - WINDOW_WIDTH) >= MS360))
            {
                sbpeak = x;
                sbloc = count - WINDOW_WIDTH;
            }
        }

        // Test for search back condition.  If a QRS is found in
        // search back update the QRS buffer and det_thresh.

        if ((count > sbcount) && (sbpeak > (det_thresh >> 1)))
        {
            qrsdet_updateQ(sbpeak);

            // Update RR Interval estimate and search back limit

            qrsdet_updateRR(sbloc);

            QrsDelay = count = count - sbloc;
            QrsDelay += FILTER_DELAY;
            sbpeak = 0;
        }
    }

    // printf("Test");
    /*             byteTwo |= (QrsDelay & 0x3F);//0F = 15
  byteTwo += 0xC0; //untersten 6 Bit der gewandelten 12 Bit in ByteTwo, 11 zu Beginn
  temp2 = QrsDelay >> 6;   //6 Bit rechtsshift
  byteOne |= (temp2 & 0x3F);//3C = 60
  byteOne += 0x80;//0x80;///obersten 6 Bit der gewandelten 12 Bit in ByteOne, 10 zu Beginn
  send_charCOM(byteOne);
  send_charCOM(byteTwo);
  temp = 0x0000;
  temp2 = 0x0000;
  byteTwo = 0x00;
  byteOne = 0x00;
  signal = 0;*/

    return (QrsDelay);
}


/**************************************************************************
*  UpdateQ takes a new QRS peak value and updates the QRS mean estimate
*  and detection threshold.
**************************************************************************/

void qrsdet_updateQ(int newQ)
{

    QSum -= Q7;
    Q7 = Q6;
    Q6 = Q5;
    Q5 = Q4;
    Q4 = Q3;
    Q3 = Q2;
    Q2 = Q1;
    Q1 = Q0;
    Q0 = newQ;
    QSum += Q0;

    det_thresh = QSum - NSum;
    det_thresh = NSum + (det_thresh >> 1) - (det_thresh >> 3);

    det_thresh >>= 3;
}


/**************************************************************************
*  UpdateN takes a new noise peak value and updates the noise mean estimate
*  and detection threshold.
**************************************************************************/

void qrsdet_updateN(int newN)
{
    NSum -= N7;
    N7 = N6;
    N6 = N5;
    N5 = N4;
    N4 = N3;
    N3 = N2;
    N2 = N1;
    N1 = N0;
    N0 = newN;
    NSum += N0;

    det_thresh = QSum - NSum;
    det_thresh = NSum + (det_thresh >> 1) - (det_thresh >> 3);

    det_thresh >>= 3;
}

/**************************************************************************
*  UpdateRR takes a new RR value and updates the RR mean estimate
**************************************************************************/

void qrsdet_updateRR(int newRR)
{
    RRSum -= RR7;
    RR7 = RR6;
    RR6 = RR5;
    RR5 = RR4;
    RR4 = RR3;
    RR3 = RR2;
    RR2 = RR1;
    RR1 = RR0;
    RR0 = newRR;
    RRSum += RR0;

    sbcount = RRSum + (RRSum >> 1);
    sbcount >>= 3;
    sbcount += WINDOW_WIDTH;
}


/**************************************************************
* peak() takes a datum as input and returns a peak height
* when the signal returns to half its peak height, or it has been
* 95 ms since the peak height was detected.
**************************************************************/


signed int qrsdet_peak(signed int datum, int init)
{
    static signed int max = 0, lastDatum;
    static int timeSinceMax = 0;
    signed int pk = 0;

    if (init)
    {
        max = 0;
        timeSinceMax = 0;
        return (0);
    }

    if (timeSinceMax > 0)
        ++timeSinceMax;

    if ((datum > lastDatum) && (datum > max))
    {
        max = datum;
        if (max > 2)
            timeSinceMax = 1;
    }

    else if (datum < (max >> 1))
    {
        pk = max;
        max = 0;
        timeSinceMax = 0;
    }

    else if (timeSinceMax > MS95)
    {
        pk = max;
        max = 0;
        timeSinceMax = 0;
    }
    lastDatum = datum;
    return (pk);
}
