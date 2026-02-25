//
// Copyright 2019 EchoNous Inc.
//
//
/***********************************************************************     
*
*  File Name:     EN_ApiCalc
*
*  Purpose:       Primary source file for the EchoNous API calculation 
*				  module. Provides the coded routines for calculation of 
*				  the parameters in the output vector and setting a safe
*                 transmit drive voltage. 
*
************************************************************************/
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "ApiCalculator.h"  

#define MI_TOLERANCE        0.01F  // Tolerance used when numerically solving the estimating polynomial for MI
#define SPTA_TOLERANCE      1.0F   // Tolerance used when numerically solving the estimating polynomial for Ispta.3

/************************************************************************
* Function name:       Pii3Calculate_utp
*
* Purpose:    Calculates the derated Pulse Intensity Integral
*             (Pii.3) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Pii3:  Derated Pulse Intensity Integral contribution
*                             from a single UTP
*                                                    
* Notes:		Pii.3 is estimated by using a best fit polynomial
*
*			03/20   Added a loop to check if ref data was collected at only
*					a single voltage, if so then estimate is made
*                   using extrapolaion from that volatge to the input
*					vector transmit voltage
*                                      
*************************************************************************/
float ApiCalculator::Pii3Calculate_utp (API_TABLE_p_t pApiTable,
                          API_INVECTOR_UTP_p_t pInputVector_utp,
                          float vdrv)
{
    int utpIdx;
    float a, b, c, d;
    float max_vdrv, min_vdrv;
	float numerator, denominator;
    float pii_3_estimate; 

    utpIdx = pInputVector_utp->utp;

    a = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_A;
    b = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_B;
    c = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_C;
    d = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_D;

	min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
    max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;

	if(min_vdrv == max_vdrv)   //  If True, this means ref data is at only the ref vDrv
	{
	numerator = pApiTable->api_utpid_table[utpIdx]->pii_3_ref *
                vdrv * vdrv *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr;

	denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref *
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref;

	assert(denominator != 0.0F);

	pii_3_estimate = numerator/denominator;
	}
	else	// Ref data taken at multiple voltages, so use polynimial estimation
	{

	if(vdrv >= pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv) //Extrapolate up from the max ref voltage
	{
		max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;
		assert(max_vdrv != 0.0F);
		pii_3_estimate = (CUBIC(a, b, c, d, max_vdrv) * vdrv * vdrv)/
                     (max_vdrv * max_vdrv);
	}
	else if (vdrv <= pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv) //Extrapolate down from the min ref voltage
	{
		min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
		assert(min_vdrv != 0.0F);
		pii_3_estimate = (CUBIC(a, b, c, d, min_vdrv) * vdrv * vdrv)/
                     (min_vdrv * min_vdrv);
	}
	else   //  Use polynomial estimation
	{
		pii_3_estimate = CUBIC(a, b, c, d, vdrv);
	}
    
	pii_3_estimate = pii_3_estimate *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
	}
                   
    return (float) pii_3_estimate;
}

/************************************************************************
* Function name:       Pii3Calculate
*
* Purpose:    Calculates the net derated Pulse Intensity Integral
*             (Pii.3) for an input vector.  Sums the contributions from
*             each UTP within the input vector
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector:  Pointer to an input vector 
*
* Returns      float  Pii3:  Aggregate derated Pulse Intensity Integral
*                             for a given input vector
*                                                    
*************************************************************************/
float ApiCalculator::Pii3Calculate (API_TABLE_p_t pApiTable,
                      API_INVECTOR_p_t pInputVector)
{
    float Pii3 = 0.0F;
    float txVdrv;
    int index;
	txVdrv = pInputVector->txVdrv;
    for(index = 0; index < pInputVector->numUtps; index++)
    {
        Pii3 += Pii3Calculate_utp(pApiTable,
			&pInputVector->api_invector_utp[index], txVdrv);
    }

    return (float) Pii3;
}

/************************************************************************
* Function name:       Pii0Calculate_utp
*
* Purpose:    Calculates the non-derated Pulse Intensity Integral
*             (Pii.0) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Pii0:  Non-derated Pulse Intensity Integral contribution
*                             from a single UTP
*                                                    
* Notes:
*                                       2
*                      (Vdrv *Vdrv_corr)    
*       (Pii.0_ref)  * -------------2 
*                          vdrv_ref
*
*************************************************************************/
float ApiCalculator::Pii0Calculate_utp (API_TABLE_p_t pApiTable,
                          API_INVECTOR_UTP_p_t pInputVector_utp,
                          float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio;

    utpIdx = pInputVector_utp->utp;

    numerator = pApiTable->api_utpid_table[utpIdx]->pii_0_ref * 
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref;

    assert(denominator != 0.0F);

    ratio = numerator/denominator;  

    return (float) ratio;
}

/************************************************************************
* Function name:       Pii0Calculate
*
* Purpose:    Calculates the net non-derated Pulse Intensity Integral
*             (Pii.0) for an input vector.  Sums the contributions from
*             each UTP within the input vector
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector:  Pointer to an input vector 
*
* Returns      float  Pii0:  Aggregate non-derated Pulse Intensity Integral
*                             for a given input vector
*                                                    
* Notes:
*
*************************************************************************/
float ApiCalculator::Pii0Calculate (API_TABLE_p_t pApiTable,
                      API_INVECTOR_p_t pInputVector)
{
    float Pii0 = 0.0F;
    float vdrv;
    int index;

	vdrv = pInputVector->txVdrv;

    for( index = 0; index < pInputVector->numUtps; index++)
    {
        Pii0 += Pii0Calculate_utp(pApiTable,
			&pInputVector->api_invector_utp[index], vdrv);
    }

    return Pii0;
}

/************************************************************************
* Function name:       SPTA3UnscannedCalculate_utp
*
* Purpose:    Calculates the derated Spatial Peak Temporal Average 
*             Intensity (Ispta.3) for a single UTP that
*             is operating in an unscanned mode. 
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Spta3:  Derated Spatial Peak Temporal Average 
*                            Intensity (Ispta.3) contribution from a 
*                            single unscanned mode UTP
*                                                    
* Notes:
*                        
*       Pii.3 * prf (pulse repitition frequency)
*
*************************************************************************/
float ApiCalculator::SPTA3UnscannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
    float Pii3 = Pii3Calculate_utp (pApiTable, pInputVector_utp, vdrv);

    return Pii3 * pInputVector_utp->prf;
}

/************************************************************************
* Function name:       Sii3Calculate_utp
*
* Purpose:    Calculates the derated Scan Intensity Integral
*             (Sii.3) given a single utp and the associated
*             input vector parameters for that utp
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_utp_p_t pInputVector_utp:  A single utp that is
*                                          part of an input vector that
*                                          may be composed of multiple utps
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Sii3:  Derated Scan Intensity Integral contribution
*                             from a single utp
*                                                    
* Notes:		Sii.3 is estimated by using a best fit polynomial
*
*			03/20   Added a loop to check if ref data was collected at only
*					a single voltage, if so then estimate is made
*                   using extrapolaion from that volatge to the input
*					vector transmit voltage
*
*************************************************************************/
float ApiCalculator::Sii3Calculate_utp (API_TABLE_p_t pApiTable,
                          API_INVECTOR_UTP_p_t pInputVector_utp,
                          float vdrv)
{
    int utpIdx;
    float a, b, c, d, numerator, denominator;
    float max_vdrv, min_vdrv;
    float sii_3_estimate; 

    utpIdx = pInputVector_utp->utp;

    a = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_A;
    b = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_B;
    c = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_C;
    d = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_D;

	min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
    max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;

	if(min_vdrv == max_vdrv)   //  If True, this means ref data is at only the ref vDrv
	{
	numerator = pApiTable->api_utpid_table[utpIdx]->sii_3_ref *
                vdrv * vdrv *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr;

	denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref *
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref;

	assert(denominator != 0.0F);

	sii_3_estimate = numerator/denominator;
	}
	else   	// Ref data taken at multiple voltages, so use polynimial estimation
	{
	if(vdrv >= pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv) //Extrapolate up from the max ref voltage
	{
		assert( max_vdrv != 0.0F);
		sii_3_estimate = (CUBIC(a, b, c, d, max_vdrv) * vdrv * vdrv)/
                         (max_vdrv * max_vdrv);
	}
	else if (vdrv <= pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv) //Extrapolate down from the min ref voltage
	{
		assert( min_vdrv != 0.0F);
		sii_3_estimate = (CUBIC(a, b, c, d, min_vdrv) * vdrv * vdrv)/
                         (min_vdrv * min_vdrv);
	}
	else   //  Use polynomial estimation
	{
		sii_3_estimate = CUBIC(a, b, c, d, vdrv);
	}

	numerator = sii_3_estimate *
                pInputVector_utp->density;          
                /*pApiTable->api_utpid_table[utpIdx]->vdrv_corr *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr;*/
                   
	denominator = pApiTable->api_utpid_table[utpIdx]->ld_ref;

	assert(denominator != 0.0F);

	sii_3_estimate = numerator/denominator;
	}

    return (float) sii_3_estimate;
}

/************************************************************************
* Function name:       SPTA3ScannedCalculate_utp
*
* Purpose:    Calculates the derated Spatial Peak Temporal Average 
*             Intensity (Ispta.3) for a single utp that
*             is operating in scanned mode. 
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_utp_p_t pInputVector_utp:  A single utp that is
*                                          part of an input vector that
*                                          may be composed of multiple utps
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Spta3:  Derated Spatial Peak Temporal Average 
*                            Intensity (Ispta.3) contribution from a 
*                            single scanned mode utp
*                                                    
* Notes:
*                        
*       Sii.3 * fr (frame rate) * ppl (pulses per line)
*
*************************************************************************/
float ApiCalculator::SPTA3ScannedCalculate_utp (API_TABLE_p_t pApiTable,
                          API_INVECTOR_UTP_p_t pInputVector_utp,
                          float vdrv)
{
    float Sii3 = Sii3Calculate_utp (pApiTable, pInputVector_utp, vdrv);

    return Sii3 * pInputVector_utp->fr * pInputVector_utp->ppl;
}
/************************************************************************
* Function name:       SPTA3Calculate
*
* Purpose:    Calculates the derated Spatial Peak Temporal Average 
*             Intensity (Ispta.3) for an input vector
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector:  Pointer to an input 
*                                                vector
*
* Returns      float  Spta3:  derated Spatial Peak Temporal 
*                              Average Intensity (Ispta.3)for a given 
*                              input vector 
*                                                    
* Notes:
*             Sums contribution from scanned and unscanned UTPs that
*             compose the input vector
*
*************************************************************************/
float ApiCalculator::SPTA3Calculate (API_TABLE_p_t pApiTable,
                       API_INVECTOR_p_t pInputVector)
{
    float Spta3 = 0.0F;
    float vdrv;
    int index;

	vdrv = pInputVector->txVdrv; 
    for(index = 0; index < pInputVector->numUtps; index++)
    {
 		switch (pInputVector->api_invector_utp[index].pulsMode)
		 {
			case API_Es:
			case API_Ps:
                Spta3 += SPTA3ScannedCalculate_utp(pApiTable,
                                                   &pInputVector->api_invector_utp[index], vdrv);
				break;  
			case API_Eu:
			case API_Pu:
            case API_Cu:
 				Spta3 += SPTA3UnscannedCalculate_utp(pApiTable,
                                                     &pInputVector->api_invector_utp[index], vdrv);
				break;

			default:
				assert(API_FALSE);
				break;
		}
    }
         
    return Spta3;
}

/************************************************************************
* Function name:       SPPA3Calculate_utp
*
* Purpose:    Calculates the derated Spatial Peak Pulse Average Intensity 
*             (SPPA.3) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_dtp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Sppa3:  Derated Spatial Peak Pulse Average Intensity 
*                              contribution from a single UTP
*                                                    
* Notes:
*                                      2
*                      (Vdrv *Vdrv_corr)    
*       (SPPA.3_ref) * ---------------- 2 
*                              vdrv_ref
*
*************************************************************************/
float ApiCalculator::SPPA3Calculate_utp (API_TABLE_p_t pApiTable,
                          API_INVECTOR_UTP_p_t pInputVector_utp,
                          float vdrv)
{
    int utpIdx;
    float numerator, denominator, Sppa3;

    utpIdx = pInputVector_utp->utp;

    numerator = pApiTable->api_utpid_table[utpIdx]->sppa_3_ref *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref; 
                 
    assert(denominator != 0.0F);
    Sppa3 = numerator/denominator;

    return Sppa3;
}

/************************************************************************
* Function name:       SPPA3Calculate
*
* Purpose:    Calculates the net derated Spatial Peak Pulse Average Intensity
*             (SPPA.3) for an input vector.  Finds the maximum SPPA.3
*             from each UTP within the input vector, and returns that value.
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector:  Pointer to an input vector 
*
* Returns      float  Sppa3:  Spatial Peak Pulse Average Intensity for 
*                              a given input vector
*                                                    
* Notes:
*
*************************************************************************/
float ApiCalculator::SPPA3Calculate (API_TABLE_p_t pApiTable,
                      API_INVECTOR_p_t pInputVector)
{
    float utpSppa3 = 0.0F, Sppa3 = 0.0F;
    float vdrv;
    int index;

	vdrv = pInputVector->txVdrv;

    for(index = 0; index < pInputVector->numUtps; index++)
    {
        utpSppa3 = SPPA3Calculate_utp(pApiTable, 
                                      &pInputVector->api_invector_utp[index], vdrv);
        if (utpSppa3 > Sppa3) Sppa3 = utpSppa3;
    }
    return Sppa3;
}

/************************************************************************
* Function name:       MICalculate_utp
*
* Purpose:    Calculates the Mechanical Index (MI)
*             for a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  MI:  Mechanical Index for a single UTP
*                                                    
* Notes:		MI is estimated by using a best fit polynomial
*
*			    Added a loop to check if ref data was collected at only
*					a single voltage, if so then estimate is made
*                   using extrapolaion from that volatge to the input
*					vector transmit voltage
*
*************************************************************************/
float ApiCalculator::MICalculate_utp (API_TABLE_p_t pApiTable,
                         API_INVECTOR_UTP_p_t pInputVector_utp,
                         float vdrv)
{
    int utpIdx;
    float a, b, c, d;
    float min_vdrv, max_vdrv, mi_estimate;
	float numerator, denominator;

    utpIdx = pInputVector_utp->utp;

    a = 0.0F;
    b = pApiTable->api_utpid_table[utpIdx]->mi_coeff_A;
    c = pApiTable->api_utpid_table[utpIdx]->mi_coeff_B;
    d = pApiTable->api_utpid_table[utpIdx]->mi_coeff_C;

	min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
    max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;

	if(min_vdrv == max_vdrv)   //  If True, this means ref data is at only the ref vDrv
	{
	numerator = pApiTable->api_utpid_table[utpIdx]->mi_3_ref *
                vdrv *
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr;

	denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref;

	assert(denominator != 0.0F);

	mi_estimate = numerator/denominator;
	}
	else   	// Ref data taken at multiple voltages, so use polynimial estimation
	{

	if(vdrv >= pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv) //Extrapolate up from the max ref voltage
	{
		max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;
		assert(max_vdrv != 0.0F);
		mi_estimate = CUBIC(a, b, c, d, max_vdrv) * vdrv/max_vdrv;
	}
	else if (vdrv <= pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv) //Extrapolate down from the min ref voltage
	{
		min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
		assert(min_vdrv != 0.0F);
		mi_estimate = CUBIC(a, b, c, d, min_vdrv) * vdrv/min_vdrv;
	}
	else   //  Use polynomial estimation
	{
		mi_estimate = CUBIC(a, b, c, d, vdrv);
	}

	mi_estimate = mi_estimate * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
	}

    return mi_estimate;
}

/************************************************************************
* Function name:       MICalculate
*
* Purpose:    Calculates the Mechanical Index
*             (MI) for an input vector.  Finds the maximum MI
*             from each UTP within the input vector, and returns that value.
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector:  Pointer
*                                          to a single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*
* Returns      float  MI:  Derated Mechanical Index for a 
*                          given input vector
*                                                    
* Notes:
*              Finds the max MI of all UTPs that compose the input vector
*
*************************************************************************/
float ApiCalculator::MICalculate (API_TABLE_p_t pApiTable,
                     API_INVECTOR_p_t pInputVector)
{
    float utpMI;
    float MI = 0.0F;
    float vdrv;
    int index;

    for(index = 0; index < pInputVector->numUtps; index++)
    {
        vdrv = pInputVector->txVdrv;	  

        utpMI = MICalculate_utp(pApiTable, &pInputVector->api_invector_utp[index], vdrv);
        if (utpMI > MI) MI = utpMI;
    }
    return MI;
}

/************************************************************************
* Function name:       Imax3Calculate_utp
*
* Purpose:    Calculates the maximum derated half cycle intensity (Imax.3)
*             for a single UTP and the associated input vector parameters 
*			  for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_dtp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  Imax3:  Derated max intensity for a single UTP
*                                                    
* Notes:
*                                 
*                                       2
*                      (Vdrv *Vdrv_corr)    
*       (Imax.3_ref) * ---------------- 2 
*                               vdrv_ref
*
*************************************************************************/
float ApiCalculator::Imax3Calculate_utp (API_TABLE_p_t pApiTable,
                         API_INVECTOR_UTP_p_t pInputVector_utp,
                         float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio;

    utpIdx = pInputVector_utp->utp;

    numerator = pApiTable->api_utpid_table[utpIdx]->imax_3_ref *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref;

    assert(denominator != 0.0F);

    ratio = numerator/denominator;

    return ratio;
}

/************************************************************************
* Function name:       Imax3Calculate
*
* Purpose:    Calculates the derated maximum half cycle Intensity
*             (Imax.3) for an input vector.  Finds the maximum Imax.3
*             from each UTP within the input vector, and returns that value.
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector:  Pointer
*                                          to a single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*
* Returns      float  Imax3:  Derated max intensity for a 
*                            given input vector
*                                                    
* Notes:
*              Finds the max Imax.3 of all UTPs that compose the input vector
*
*************************************************************************/
float ApiCalculator::Imax3Calculate (API_TABLE_p_t pApiTable,
                     API_INVECTOR_p_t pInputVector)
{
    float vdrv, utpImax3 = 0.0F, Imax3 = 0.0F;
    int index;

	vdrv = pInputVector->txVdrv;
    for(index = 0; index < pInputVector->numUtps; index++)
    {
        utpImax3 = Imax3Calculate_utp(pApiTable, &pInputVector->api_invector_utp[index], vdrv);
        if (utpImax3 > Imax3) Imax3 = utpImax3;
    }
    return Imax3;
}

/************************************************************************
* Function name:       PowerUnscannedCalculate_utp
*
* Purpose:    Calculates the unscanned mode acoustic power
*             (W0u) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  W0u:   Unscanned mode acoustic power
*                             from a single UTP
*                                                    
* Notes:
*                                          2
*                         (Vdrv *Vdrv_corr)    
*       (W0u_ref) * prf * ----------------- 2 
*                                   vdrv_ref
*
*************************************************************************/
float ApiCalculator::PowerUnscannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
	int utpIdx;
    float numerator, denominator, ratio;

    utpIdx = pInputVector_utp->utp;

    numerator = pApiTable->api_utpid_table[utpIdx]->w0u_ref * 
                pInputVector_utp->prf *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref; 
	
	/**  For unscanned modes, set the mask fractions equal to 1.0  **/
	pApiTable->api_utpid_table[utpIdx]->w0sFrac = 1.0F;
	pApiTable->api_utpid_table[utpIdx]->w01sFrac = 1.0F;
	
	assert(denominator != 0.0F);

    ratio = numerator/denominator;

    return (float) ratio;
}

/************************************************************************
* Function name:       PowerScannedCalculate_utp
*
* Purpose:    Calculates the scanned mode acoustic power
*             (W0s) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*             int utpIdx:  Index into the UTP table for the UTP being 
*                            evaluated
*             int CalcPowerFractions:  Boolean that determines whether
*                                       the power mask fractions need to be
*                                       calculated.
*
* Returns      float  W0s:   Scanned mode acoustic power
*                            from a single UTP
*                                                    
* Notes:
*                                                     2
*                                    (Vdrv *Vdrv_corr)    
*       (W0u_ref) * prf * W0sFrac * -------------------2 
*                                              vdrv_ref
*************************************************************************/
float ApiCalculator::PowerScannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv,
                                    int utpIdx,
                                    int CalcPowerFractions)
{
    float numerator, denominator, ratio;

    utpIdx = pInputVector_utp->utp;

    if(CalcPowerFractions)   /**  If the power fractions for the UTP need to be calculated and initialized **/
    {
        GetMaskFractions(pInputVector_utp, pApiTable, pApiTable->api_utpid_table[utpIdx]); /* then set them */
    }

    /**  Ensure that the W0sFrac was properly set  **/
    assert(pApiTable->api_utpid_table[utpIdx]->w0sFrac != -1.0F);

    numerator = pApiTable->api_utpid_table[utpIdx]->w0u_ref *
                pApiTable->api_utpid_table[utpIdx]->w0sFrac * 
                pInputVector_utp->prf *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref; 

	assert(denominator != 0.0F);

    ratio = numerator/denominator;

    return (float) ratio;
}

/************************************************************************
* Function name:       AcousticPowerCalculate
*
* Purpose:    Calculates the total acoustic power (W0) for an input vector
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector: pointer to an input vector
*
* Outputs:    float *W0   pointer to the total acoustic output power
*             float *W01  pointer to the acoustic output power 
*                          transmitted from the center 1 cm^2 of the 
*                          transducer face 
*                                                    
* Notes:
*             Sums contribution from scanned and unscanned UTPs that
*             compose the input vector
*
*************************************************************************/
void ApiCalculator::AcousticPowerCalculate (API_TABLE_p_t pApiTable,
                                       API_INVECTOR_p_t pInputVector,
                                       float *W0, float *W01)
{
    float W0_utp, W01_utp, vdrv;
    int index, utpIdx;

    for(index = 0; index < pInputVector->numUtps; index++)
    {
        utpIdx = pInputVector->api_invector_utp[index].utp; 
	
        vdrv = pInputVector->txVdrv;	  
		
        switch (pInputVector->api_invector_utp[index].pulsMode)
        {
            case API_Es:
            case API_Ps:
                W0_utp = PowerScannedCalculate_utp(pApiTable,
                                                   &pInputVector->api_invector_utp[index],
                                                   vdrv, utpIdx, API_TRUE);

                /**  Ensure that the W01sFrac was properly set  **/
                assert(pApiTable->api_utpid_table[utpIdx]->w01sFrac != -1.0F);
                assert(pApiTable->api_utpid_table[utpIdx]->w0sFrac != 0.0F);

                W01_utp = W0_utp * 
                          pApiTable->api_utpid_table[utpIdx]->w01sFrac; 

                *W0 += W0_utp;
                *W01 += W01_utp;

                break;

            case API_Eu:
            case API_Pu:
            case API_Cu:
                W0_utp = PowerUnscannedCalculate_utp(pApiTable,
                                                     &pInputVector->api_invector_utp[index],
                                                     vdrv);
                *W0 += W0_utp;
                if(pApiTable->api_utpid_table[utpIdx]->aaprt_ref > 1.0F) /*  unscanned aperture is > 1 cm^2  */
                {
                    *W01 += W0_utp / pApiTable->api_utpid_table[utpIdx]->aaprt_ref;
                }
                else
                {
                    *W01 += W0_utp;
                }
                break;
            default:
                assert(API_FALSE);
                break;
        }
    }
}

/************************************************************************
* Function name:       TISasCalculate_utp
*
* Purpose:    Calculates the  soft tissue thermal index
*             at surface (TISas) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  TISas:   Soft tissue thermal index
*                                at surface.  
*								                    
* Notes:
*                                                          2
*     (W0u * cFreq)                      (Vdrv * Vdrv_corr)    
*     -------------  (W01sFrac) * prf * --------------------
*         210.0                          vdrv_ref * vdrv_ref
*
*       Note, Per IEC 62359, both scanned and unscanned TISas values are
*			  calculated in the same manner       
*
*************************************************************************/
float ApiCalculator::TISasCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio, TISas_factor;

	utpIdx = pInputVector_utp->utp;

	TISas_factor = (pApiTable->api_utpid_table[utpIdx]->w0u_ref * 
		            pApiTable->api_utpid_table[utpIdx]->cFreq)/210.0F;


	/** check that the W01sFrac has already been calculated  **/
    assert(pApiTable->api_utpid_table[utpIdx]->w01sFrac != -1.0F);

    numerator = TISas_factor * pApiTable->api_utpid_table[utpIdx]->w01sFrac *
                pInputVector_utp->prf *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref; 

	assert(denominator != 0.0F);

    ratio = numerator/denominator;

    return (float) ratio;
}

/************************************************************************
* Function name:       TISasCalculate
*
* Purpose:    Calculates the soft tissue thermal index at surface (TIsAS)
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector: pointer to an input vector
*
* Returns      float  TISas:  Soft tissue thermal index at surface
*                                                    
* Notes:
*             Sums contributions from the UTPs that
*             compose the input vector
*
*************************************************************************/
float ApiCalculator::TISasCalculate (API_TABLE_p_t pApiTable,
                      API_INVECTOR_p_t pInputVector)
{
    float TISas = 0.0F, vdrv;
    int index;

	vdrv = pInputVector->txVdrv;
    
    for(index = 0; index < pInputVector->numUtps; index++)
    {
		TISas += TISasCalculate_utp(pApiTable, &pInputVector->api_invector_utp[index],
                                    vdrv);
    }
    return TISas;
}

/************************************************************************
* Function name:       TISbsUnscannedCalculate_utp
*
* Purpose:    Calculates the unscanned soft tissue thermal index
*             below surface (TISbs) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  TISbsu:   Unscanned mode soft tissue thermal index
*                                below surface                    
* Notes:
*                                            2
*                            (Vdrv *Vdrv_corr)    
*       (TISbs_ref) * prf *  ----------------- 2 
*                              vdrv_ref
*
*************************************************************************/
float ApiCalculator::TISbsUnscannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio;

    utpIdx = pInputVector_utp->utp;

    numerator = pApiTable->api_utpid_table[utpIdx]->TISbs_ref *
                pInputVector_utp->prf *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref; 

	assert(denominator != 0.0F);

    ratio = numerator/denominator;

    return (float) ratio;
}

/************************************************************************
* Function name:       TISbsCalculate
*
* Purpose:    Calculates the soft tissue thermal index below surface (TISbs)
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector: pointer to an input vector
*
* Returns      float  TISbs:  Soft tissue thermal index below surface
*                                                    
* Notes:
*             Sums contribution from scanned and unscanned UTPs that
*             compose the input vector.  Note that per IEC 62359
*			  Soft Tissue below surface for scanned modes is
*			  calculated the same as soft tissue thermal index at surface
*
*************************************************************************/
float ApiCalculator::TISbsCalculate (API_TABLE_p_t pApiTable,
                      API_INVECTOR_p_t pInputVector)
{
    float TISbs = 0.0F, vdrv;
    int index;
   
	vdrv =pInputVector->txVdrv;  
    for(index = 0; index < pInputVector->numUtps; index++)
    {
        switch (pInputVector->api_invector_utp[index].pulsMode)
        {
            case API_Es:
            case API_Ps:
				TISbs += TISasCalculate_utp (pApiTable,
                                    &pInputVector->api_invector_utp[index],
                                    vdrv);
                break;
            case API_Eu:
            case API_Pu:
            case API_Cu:
                TISbs += TISbsUnscannedCalculate_utp(pApiTable,
                                    &pInputVector->api_invector_utp[index],
                                    vdrv);
                break;
            default:
                assert(API_FALSE);
                break;
        }
    }
    return TISbs;
}

/************************************************************************
* Function name:       TIBbsUnscannedCalculate_utp
*
* Purpose:    Calculates the unscanned bone thermal index
*             below surface (TIBbsu) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  TIBbs:   Unscanned mode soft tissue thermal index
*                                below surface                    
* Notes:
*                                           2
*                          (Vdrv *Vdrv_corr)    
*       (TIBbs_ref) * prf* ------------- 2 
*                              vdrv_ref
*
*************************************************************************/
float ApiCalculator::TIBbsUnscannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio;

    utpIdx = pInputVector_utp->utp;

    numerator = pApiTable->api_utpid_table[utpIdx]->TIBbs_ref *
                pInputVector_utp->prf *
                vdrv * vdrv * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr * 
                pApiTable->api_utpid_table[utpIdx]->vdrv_corr; 
                       
    denominator = pApiTable->api_utpid_table[utpIdx]->vdrv_ref * 
                  pApiTable->api_utpid_table[utpIdx]->vdrv_ref; 

	assert(denominator != 0.0F);

    ratio = numerator/denominator;

    return (float) ratio;
}

/************************************************************************
* Function name:       TIBbsCalculate
*
* Purpose:    Calculates the bone thermal index below surface (TIbBS)
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector: pointer to an input vector
*
* Returns      float  TIBbs:  Bone thermal index below surface
*                                                    
* Notes:
*             Sums contribution from scanned and unscanned UTPs that
*             compose the input vector
*
*************************************************************************/
float ApiCalculator::TIBbsCalculate (API_TABLE_p_t pApiTable,
                      API_INVECTOR_p_t pInputVector)
{
    float vdrv;
    float TIBbs = 0.0F;	   //  Final TIBbs value
    float TIbBSs = 0.0F;   //  TIBbs derived from scanned UTPs
    float TIbBSu = 0.0F; 	   //  TIBbs derived from unscanned UTPs  
    int index;

	vdrv =pInputVector->txVdrv;
    for(index = 0; index < pInputVector->numUtps; index++)
    {
        switch (pInputVector->api_invector_utp[index].pulsMode)
        {
            case API_Es:  /* for scanned modes, use the soft tissue at surface model */
            case API_Ps:
                TIbBSs += TISasCalculate_utp(pApiTable,
                                             &pInputVector->api_invector_utp[index],
                                             vdrv);
                break;
            case API_Eu:
            case API_Pu:
            case API_Cu:
                TIbBSu += TIBbsUnscannedCalculate_utp(pApiTable,
                                                   &pInputVector->api_invector_utp[index],
                                                   vdrv);
                break;
            default:
                assert(API_FALSE);
                break;
        }
        TIBbs = TIbBSu > TIbBSs ? TIbBSu : TIbBSs;  
    }
    return TIBbs;
}

/**************************************************************
* Function name:       TICasUnscannedCalculate_utp
*
* Purpose:    Calculates the unscanned cranial thermal index
*             at surface (TIcASu) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  TIcASu:   Unscanned mode cranial thermal index
*                                at surface                    
* Notes:
*
*
*		Note:  TIBas = TIC
*************************************************************************/
float ApiCalculator::TICasUnscannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio, aaprt;

    utpIdx = pInputVector_utp->utp;
	
	aaprt = pApiTable->api_utpid_table[utpIdx]->aaprt_ref;

    numerator = PowerUnscannedCalculate_utp(pApiTable, pInputVector_utp, vdrv); 

	assert(aaprt >= 0.0F);
    denominator = TIC_DENOM_CONST * (float) sqrt(aaprt);
      
    assert(denominator != 0.0F);

    ratio = numerator/denominator;  

    return (float) ratio;
}


/************************************************************************
* Function name:       TICasScannedCalculate_utp
*
* Purpose:    Calculates the scanned cranial thermal index
*             at surface (TICas) given a single UTP and the associated
*             input vector parameters for that UTP
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t pInputVector_utp:  A single UTP that is
*                                          part of an input vector that
*                                          may be composed of multiple UTPs
*             float vdrv:  Drive voltage for the input vector
*
* Returns      float  TICas:   Scanned mode cranial thermal index
*                                at surface                    
* Notes:
*                                                              
*************************************************************************/
float ApiCalculator::TICasScannedCalculate_utp (API_TABLE_p_t pApiTable,
                                    API_INVECTOR_UTP_p_t pInputVector_utp,
                                    float vdrv)
{
    int utpIdx;
    float numerator, denominator, ratio;
    float asaprt = 0.0F;

    utpIdx = pInputVector_utp->utp;

    /*  Set the numerator to the scanned acoustic power, but there is no need	   * 
     *   to calculate the power mask fractions because it has already done in the  *
     *   first call to PowerScannedCalculate_utp from the within the               *
     *   AcousticPowerCalculate function.                                          */

    numerator = PowerScannedCalculate_utp (pApiTable, pInputVector_utp, vdrv, utpIdx, API_FALSE); 

    asaprt = GetScannedApertureArea(pApiTable, pInputVector_utp, utpIdx);  

	assert(asaprt >= 0.0F);
    denominator = TIC_DENOM_CONST * (float) sqrt(asaprt);
      
    assert(denominator != 0.0F);

    ratio = numerator/denominator;  

    return (float) ratio;
}

/************************************************************************
* Function name:       TIcCalculate
*
* Purpose:    Calculates the cranial thermal index (TIc)
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_p_t pInputVector: pointer to an input vector
*
* Returns      float  TIC:  Cranial thermal index
*                                                    
* Notes:
*             Sums contribution from scanned and unscanned UTPs that
*             compose the input vector
*
*************************************************************************/
float ApiCalculator::TIcCalculate(API_TABLE_p_t pApiTable,
                    API_INVECTOR_p_t pInputVector)
{
    float TIC = 0.0F, vdrv;
    int index;
 
	vdrv =pInputVector->txVdrv;

    for(index = 0; index < pInputVector->numUtps; index++)
    {
        switch (pInputVector->api_invector_utp[index].pulsMode)
        {
            case API_Es:
            case API_Ps:
                TIC += TICasScannedCalculate_utp(pApiTable,
                                                 &pInputVector->api_invector_utp[index],
                                                 vdrv);
                break;
            case API_Eu:
            case API_Pu:
            case API_Cu:
                TIC += TICasUnscannedCalculate_utp(pApiTable,
                                                   &pInputVector->api_invector_utp[index],
                                                   vdrv);
                break;
            default:
                assert(API_FALSE);
                break;
        }
    }
    return TIC;
}

/************************************************************************
* Function name:       API_CalculateOutputVector
*
* Purpose:    Calls all the calculation methods required to calculate
*          the output vector parameters and then populates the output 
*          vector structure with the values.
*
* Inputs:     API_INVECTOR_p_t pInvector   Pointer to input vector
*             API_TABLE_p_t pApiTable:  Pointer to API table
*             API_OUTVECTOR_p_t pApiOutVector:  Pointer to the output vector
*
*             int  updateVlimits Boolean that specifies whether the 5 voltage limits
*                                will be recalculated.  Normally the first time
*                                that this function is called, this Boolean is
*                                set TRUE so that the voltage limts are calculated.
*                                The second time this function is called, the input
*                                vector drive voltage has already been set to the 
*                                lowest of the voltage limits so the Boolean is set
*                                FALSE and the voltage limits are not recalculated.
*
* Returns      
*                                                    
* Notes:
*                                                       
*************************************************************************/
void ApiCalculator::CalculateOutputVector (API_INVECTOR_p_t pApiInvector,
                        API_TABLE_p_t pApiTable,
                        API_OUTVECTOR_p_t pApiOutvector,
                        int updateVlimits)
{
    float W0 = 0.0F, W01 = 0.0F; /**  Acoustic output power variables **/ 
    pApiOutvector->spta_3 = SPTA3Calculate (pApiTable, pApiInvector);
    pApiOutvector->sppa_3 = SPPA3Calculate (pApiTable, pApiInvector);
    pApiOutvector->pii_3 = Pii3Calculate (pApiTable, pApiInvector);
    pApiOutvector->pii_0 = Pii0Calculate (pApiTable, pApiInvector);   
    pApiOutvector->mi = MICalculate (pApiTable, pApiInvector);
    pApiOutvector->imax_3 = Imax3Calculate (pApiTable, pApiInvector);
    AcousticPowerCalculate (pApiTable, pApiInvector, &W0, &W01);
    pApiOutvector->w0 = W0; 
    pApiOutvector->w01 = W01;    
    pApiOutvector->tisas = TISasCalculate (pApiTable, pApiInvector);
    pApiOutvector->tisbs = TISbsCalculate (pApiTable, pApiInvector);  
    pApiOutvector->tibbs = TIBbsCalculate (pApiTable, pApiInvector);
    pApiOutvector->tic = TIcCalculate (pApiTable, pApiInvector);
    pApiOutvector->tis_display =  pApiOutvector->tisas > pApiOutvector->tisbs ? pApiOutvector->tisas : pApiOutvector->tisbs;
    pApiOutvector->tib_display =  pApiOutvector->tibbs > pApiOutvector->tisas ? pApiOutvector->tibbs : pApiOutvector->tisas;
    pApiOutvector->tic_display =  pApiOutvector->tic; 
    if(updateVlimits)
    {
		pApiOutvector->vdrvCmd = GetCommandVdrv(pApiInvector, pApiTable, pApiOutvector);
    }
}

/************************************************************************
* Function name:       SPTA_VlimCalculate
*
* Purpose:    Calculates the maximum drive voltage based on the limit
*             imposed on derated spatial-peak temporal-average acoustic
*             intensity (Ispta.3)
*
* Inputs:     API_INVECTOR_p_t pInvector   Pointer to input vector
*             API_TABLE_p_t pApiTable:  Pointer to API table
*             API_OUTVECTOR_p_t pApiOutVector:  Pointer to the output
*                                               vector
*
* Returns      float  VdrvLimSpta:  Drive voltage limit
*                                                    
* Notes:
*                                                       
*************************************************************************/
float ApiCalculator::SPTA_VlimCalculate (API_INVECTOR_p_t pInvector,
                           API_TABLE_p_t pApiTable,
                           API_OUTVECTOR_p_t pApiOutvector)
{
    int utpIdx, index, coeffIndex;
    float VdrvLimSpta = 0.0F;
    float coeffs[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    float coeffs_utp[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    float minRefVdrv = 0.0F, maxRefVdrv = 999.0F;
    float spta_limit, vdrv, min_vdrv, max_vdrv, ratio;
	int singleVoltage = API_FALSE;

    spta_limit = pApiTable->api_table_header.SPTA_lim;

    for(index = 0; index < pInvector->numUtps; index++)  // Check each UTP to see if ref data was collected at only one voltage
	{
		utpIdx = pInvector->api_invector_utp[index].utp;
		min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
	max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;

		if(min_vdrv == max_vdrv)   //  If True, this means ref data is taken at only the ref vDrv
			singleVoltage = API_TRUE;
	}

	if (singleVoltage)		 //  Single reference voltage used in at least one UTP
	{
		ratio = spta_limit/pApiOutvector->spta_3;
		assert(ratio > 0.0F);
		VdrvLimSpta = (float)(sqrt(ratio) * pInvector->txVdrv);   //  Now quadratically extrapolate for Vdrv that yields the SPTA.3 limit
	}
	else
    {
        //  First create the composite polynomial for estimating the SPTA.3 by summing the contribution from each UTP in the input vector
        for(index = 0; index < pInvector->numUtps; index++)
        {
            minRefVdrv = 0.0F;
            maxRefVdrv = 999.0F;
            utpIdx = pInvector->api_invector_utp[index].utp;

            minRefVdrv = minRefVdrv > pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv ? minRefVdrv : pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
            maxRefVdrv = maxRefVdrv < pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv ? maxRefVdrv : pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;

            switch (pInvector->api_invector_utp[index].pulsMode)
            {
                case API_Es:
                case API_Ps:
                    coeffs_utp[0] = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_A
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    coeffs_utp[1] = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_B
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    coeffs_utp[2] = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_C
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    coeffs_utp[3] = pApiTable->api_utpid_table[utpIdx]->sii3_coeff_D
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    for(coeffIndex = 0; coeffIndex <= 3; coeffIndex++)
                    {
                        assert(pApiTable->api_utpid_table[utpIdx]->ld_ref != 0.0F);
                        coeffs[coeffIndex] += coeffs_utp[coeffIndex] * pInvector->api_invector_utp[index].density
                                                                 * pInvector->api_invector_utp[index].fr
                                                                 * pInvector->api_invector_utp[index].ppl / pApiTable->api_utpid_table[utpIdx]->ld_ref;
                    }
                    break;
                case API_Eu:
			    case API_Pu:
                    coeffs_utp[0] = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_A
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    coeffs_utp[1] = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_B
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    coeffs_utp[2] = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_C
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                    coeffs_utp[3] = pApiTable->api_utpid_table[utpIdx]->pii3_coeff_D
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr
                                * pApiTable->api_utpid_table[utpIdx]->vdrv_corr;
                
                    for(coeffIndex = 0; coeffIndex <= 3; coeffIndex++)
                    {
                        coeffs[coeffIndex] += coeffs_utp[coeffIndex] * pInvector->api_invector_utp[index].prf;
                    }
                    break;
                case API_Cu:
                    break;
            }
        }
        VdrvLimSpta = GetVdrvSolution(coeffs, spta_limit, SPTA_TOLERANCE, minRefVdrv, maxRefVdrv, API_QUAD);   //  Now solve for the Vdrv that gets the 
    }

	vdrv = pInvector->txVdrv;	 

    return (float) (VdrvLimSpta < vdrv ? VdrvLimSpta : vdrv);   // If input vector Tranmit voltage is less than the SPTA limit voltage, use the input vetor voltage
}

/************************************************************************
* Function name:       MI_VlimCalculate
*
* Purpose:    Calculates the maximum drive voltage based on the limit
*             imposed on mechanical index (MI)
*
* Inputs:     API_INVECTOR_p_t pInvector   Pointer to input vector
*             API_TABLE_p_t pApiTable:  Pointer to API table
*             API_OUTVECTOR_p_t pApiOutVector:  Pointer to the output
*                                               vector
*
* Returns      float  VdrvLimMI:  Drive voltage limit
*                                                    
* Notes:
*                                                       
*************************************************************************/
float ApiCalculator::MI_VlimCalculate (API_INVECTOR_p_t pInvector,
                           API_TABLE_p_t pApiTable,
                           API_OUTVECTOR_p_t pApiOutvector)
{
    float VdrvLimMI = 999.0F,  VdrvLimMI_utp, mi_limit, vdrv;
    float coeffs_utp[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    int index, utpIdx;
    float minRefVdrv = 0.0F, maxRefVdrv = 999.0F;
	float min_vdrv, max_vdrv, ratio;
	int singleVoltage = API_FALSE;  //  serves as a boolean flag to indicate if any of the UTPs in the input vector used just a single ref voltage

    mi_limit = pApiTable->api_table_header.MI_lim;
    
    for(index = 0; index < pInvector->numUtps; index++)  // Check each UTP to see if ref data was collected at only one voltage
	{
		utpIdx = pInvector->api_invector_utp[index].utp;
		min_vdrv = pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
	max_vdrv = pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;

		if(min_vdrv == max_vdrv)   //  If True, this means ref data is taken at only the ref vDrv
			singleVoltage = API_TRUE;  // so set the boolean flag indicating that a single voltage was used
	}


	if (singleVoltage)		 //  Single reference voltage used in at least one UTP
	{
		ratio = mi_limit/pApiOutvector->mi;
		VdrvLimMI = ratio * pInvector->txVdrv;   //  Now linearly extrapolate for Vdrv that yields the MI limit
	}
	else
    {
        //  Index through the UTPs of the input vector and determine which UTP yields the most limiting drive voltage (i.e. lowest voltage) to remain at or below the MI limit
        for(index = 0; index < pInvector->numUtps; index++)
        {
            minRefVdrv = 0.0F;
            maxRefVdrv = 999.0F;
            utpIdx = pInvector->api_invector_utp[index].utp;

            minRefVdrv = minRefVdrv > pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv ? minRefVdrv : pApiTable->api_utpid_table[utpIdx]->min_ref_vdrv;
            maxRefVdrv = maxRefVdrv < pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv ? maxRefVdrv : pApiTable->api_utpid_table[utpIdx]->max_ref_vdrv;
    
            coeffs_utp[0] = 0.0F;
		    coeffs_utp[1] = (float)(pApiTable->api_utpid_table[utpIdx]->mi_coeff_A * pApiTable->api_utpid_table[utpIdx]->vdrv_corr);
            coeffs_utp[2] = (float)(pApiTable->api_utpid_table[utpIdx]->mi_coeff_B * pApiTable->api_utpid_table[utpIdx]->vdrv_corr);
            coeffs_utp[3] = (float)(pApiTable->api_utpid_table[utpIdx]->mi_coeff_C * pApiTable->api_utpid_table[utpIdx]->vdrv_corr);
        
            VdrvLimMI_utp = GetVdrvSolution(coeffs_utp, mi_limit, MI_TOLERANCE, minRefVdrv, maxRefVdrv, API_LIN);  //  Now solve for the Vdrv that meets the MI limit

            VdrvLimMI = VdrvLimMI_utp < VdrvLimMI ? VdrvLimMI_utp : VdrvLimMI;   //  Set the lowest voltage from the derived MI vDrv values for each UTP
        }
    }
   
     vdrv = pInvector->txVdrv;	 

	 return VdrvLimMI < vdrv ? VdrvLimMI : vdrv; // If input vector Tranmit voltage is less than the MI limit voltage, use the input vetor voltage
}

/************************************************************************
* Function name:       TI_VlimCalculate
*
* Purpose:    Calculates the maximum drive voltage based on the limit
*             imposed on thermal index (TI).
*
*
* Inputs:     API_INVECTOR_p_t pInvector   Pointer to input vector
*             API_TABLE_p_t pApiTable:  Pointer to API table
*             API_OUTVECTOR_p_t pApiOutVector:  Pointer to the output
*                                               vector
*
* Returns      float  VdrvLimTI:  Drive voltage limit for Thermal Index
*
* Notes:
*
* Change History:
*   Date    Description
*  -------- -----------
*  03/20    Created
*
*************************************************************************/
float ApiCalculator::TI_VlimCalculate (API_INVECTOR_p_t pInvector,
                           API_TABLE_p_t pApiTable,
                           API_OUTVECTOR_p_t pApiOutvector)
{
    float VdrvLimTI,ti_limit, vdrv, maxTI;

	vdrv = pInvector->txVdrv;
    ti_limit = pApiTable->api_table_header.TI_lim;

    maxTI = GetMaxTI(pApiOutvector);

	assert(ti_limit/maxTI > 0.0F);

    VdrvLimTI = (float)(sqrt(ti_limit/maxTI) * vdrv);


   return (float) (VdrvLimTI < vdrv ? VdrvLimTI : vdrv);
}

/************************************************************************
* Function name:       GetMinVdrv
*
* Purpose:    Finds the mimimum voltage limit from the 5 previously
*             calculated limits (i.e. SPTA, MI, and SST) that have
*             been placed into the output vector structure.
*
* Inputs:     API_OUTVECTOR_p_t pApiOutvector   Pointer to the output vector
*
* Returns      float  minVdrv:  Mimimum drive voltage limit
*                                                    
* Notes:
*		03/2020  Added line to check to see if TI limit (ti_vlim)
*				 is the most restrictive
*                                                       
*
*************************************************************************/
float ApiCalculator::GetMinVdrv(API_OUTVECTOR_p_t pApiOutvector)
{
    float minVdrv = 1000.0F;

    if (pApiOutvector->spta_vlim < minVdrv)
    {
        minVdrv = pApiOutvector->spta_vlim;
    }
    if (pApiOutvector->mi_vlim < minVdrv)
    {
        minVdrv = pApiOutvector->mi_vlim;
    }
    if (pApiOutvector->ti_vlim < minVdrv)
    {
        minVdrv = pApiOutvector->ti_vlim;
    }

    return minVdrv;
}

/************************************************************************
* Function name:       GetCommandVdrv
*
* Purpose:    Calls functions to calculate the voltage limits for SPTA, MI, 
*             TI, EP and SST, and then returns the minimum of these 3 voltage
*             limits as the dive voltage to be commanded.
*
* Inputs:     API_VLIM_p_t pVdrvLims   Pointer to Vdrv limits
*
* Returns      float  vDrv:  drive voltage command
*                                                    
* Notes:
*                                                       
*************************************************************************/
float ApiCalculator::GetCommandVdrv(API_INVECTOR_p_t pApiInvector,
                      API_TABLE_p_t pApiTable,
                      API_OUTVECTOR_p_t pApiOutvector)
{
    float vDrv = 0.0F;

    pApiOutvector->spta_vlim = SPTA_VlimCalculate(pApiInvector, pApiTable, pApiOutvector);
    pApiOutvector->mi_vlim = MI_VlimCalculate(pApiInvector, pApiTable, pApiOutvector);
    pApiOutvector->ti_vlim = TI_VlimCalculate(pApiInvector, pApiTable, pApiOutvector);
    vDrv = GetMinVdrv(pApiOutvector);  
      
    return vDrv;
}


/************************************************************************
* Function name:       GetVdrvSolution
*
* Purpose:    This function is used to derive the transmit drive voltage 
*			  that yields the value of the acoustic parameter (i.e. SPTA.3 
*			  limit or MI.3 limit) sought. The function first checks to 
*			  see if the drive voltage required falls within 
*			  the range of reference data voltages used when data was 
*			  collected, If it does, then a polynomial interpolation is 
*			  used to derive the transmit drive voltage. If the voltage 
*			  is outside of the reference voltage range, then a regular 
*			  linear or quadratic extrapolation is used rather than the 
*			  polynomial interpolation estimate.  This precludes the 
*			  possibility of using an errant polynomial value. 
*
* Inputs:     float coeffs[]:  Estimating polynomial coefficients
*			  float acousticParamValue: Either the SPTA or MI control limit value
*			  float minRefVdrv:  Minimum voltage over which acoustic ref data was 
*								 acquired. 
*			  float maxRefVdrv:  Maximum voltage over which acoustic ref data was 
*								 acquired. 
*			  API_EXTRAP_METHOD method:  Method used to extrpolate in the event that
*								 that the vdrv solution falls outside of the range
*								 of the acoustic reference voltages.
*
* Returns  int  vdrvSolution:  Drive voltage that yields acousticParamValue
*                                                    
* Notes:
*
*************************************************************************/
float ApiCalculator::GetVdrvSolution(float coeffs[], float acousticParamValue, float tolerance, float minRefVdrv, float maxRefVdrv, API_EXTRAP_METHOD method)
{
    float lowerRefV_Value, upperRefV_Value, vdrvSolution;
    float actualParamValue, error;
    
    if((upperRefV_Value = CUBIC(coeffs[0], coeffs[1], coeffs[2], coeffs[3], maxRefVdrv)) > acousticParamValue) 
    {
        if((lowerRefV_Value = CUBIC(coeffs[0], coeffs[1], coeffs[2], coeffs[3], minRefVdrv)) < acousticParamValue)
        {
            vdrvSolution = PolySolve(coeffs, minRefVdrv, maxRefVdrv, acousticParamValue, &actualParamValue, 
                                      tolerance, &error);   // solve using polynomial interpolation
        }
        else  // The value required is lower than the minimum voltage used for ref data collection, therefore extrapolate 
        {
            if (method == API_LIN)   // use linear extrapolation
                vdrvSolution = (acousticParamValue/lowerRefV_Value) * minRefVdrv;
            else  //  use quadratic extrapolation
                vdrvSolution = (float)(sqrt(acousticParamValue/lowerRefV_Value) * minRefVdrv);
        }
    }
    else // The value required is higher than the maximum voltage used for ref data collection, therefore extrapolate 
    {
        if (method == API_LIN)   // use linear extrapolation
        {
            vdrvSolution = (acousticParamValue/upperRefV_Value) * maxRefVdrv;
        }
        else  //  use quadratic extrapolation  
        {
            vdrvSolution = (float)(sqrt(acousticParamValue/upperRefV_Value) * maxRefVdrv);
        }
    }
    return vdrvSolution;
}

/************************************************************************
* Function name:       GetMaskFractions
*
* Purpose:    For scanned operating modes it models the available aperture 
*             and 1 cm^2 mask mathematically so that the fraction of
*             unscanned output power (reported in the API table as W0u_ref)
*             can be used for power and TI calculations.  The values for 
*             the fractions are set in the pApiTable->api_utpid_table[utpIdx]->w0sFrac and 
*			  w01sFrac elements of UTP table portion of the API Table of the UPS.
*             
*
* Inputs:     
*             API_INVECTOR_UTP_p_t pInputVector_utp: An input vector for a single UTP   
*             API_TABLE_p_t pApiTable:  Pointer to API table
*             API_UTP_TABLE_ITEM_p_t pApiTable_utp:  Pointer to a single UTP
*                                    structure within the UTP ID table portion
*                                    of the API table
*
* Outputs     The scanned output power fraction and 1 cm^2 aperture 
*             fractional values are set in the API table of the UPS database 
*             (i.e. pApiTable_dtp->pApiTable->api_utpid_table[utpIdx]->w0sFrac & pApiTable_utp->w01sFrac) 
*
* Notes:
*           Fractional values are determined by simulating a scanned mode
*			aperture sliding across the array face and then calculating
*			the fraction of the amount of power in the full scanned area 
*			and the center 1 cm^2.  Assumes no apodization.
*                                                       
*
*************************************************************************/
void ApiCalculator::GetMaskFractions(API_INVECTOR_UTP_p_t pInputVector_utp,
                       API_TABLE_p_t pApiTable,
                       API_UTPID_TABLE_p_t pApiTable_utp) 
{
	float pitch = pApiTable->api_table_header.ElePitch; /* pitch is in millimeters */
 	int maxElements = pApiTable->api_table_header.NumEles;
    int apSize = pApiTable_utp->apEles;;
    int numLines = pInputVector_utp->lpFrame;
    float density = pInputVector_utp->density;
    float apex = pApiTable->api_table_header.Apex;		/*  Apex in cm  */
    API_XDUCER_CLASS xducerClass = pApiTable->api_table_header.Class;
    float elePerLine = 0.0,elePerDeg;
	int maxSweepLength, currentSlide, newSlide, numElements2Slide, tempValue,eles_per_cm;
	int apArray[300], apArraySlide[300], availAp[300], oneCmAp[300]; 
	float totalApPower, availApPower, oneCmApPower;
	int minIndex, maxIndex, minIndex_1_cm, maxIndex_1_cm, numEles;
	int apIndex = 0;

	if(xducerClass == API_PHA && apex == 0.0F)  //  If this is a phased array with a fixed (i.e. non sliding) aperture
	{
		pApiTable_utp->w0sFrac =  1.0F;   // Aperture is fixed, so no edge effects to account for
		pApiTable_utp->w01sFrac = pApiTable_utp->aaprt_ref > 1.0F ? 1/pApiTable_utp->aaprt_ref : 1.0F;  // set the 1 cm^2 fraction
	}
	else  // Not a phased array with a non-sliding aperture, so simulate an aperture slide to determine power fractions
	{
		switch (xducerClass)
		{
            case API_LA:        // Linear array
                elePerLine = 1/(pitch * density);	 /* pitch in mm, density in lines/mm  */
                break;
            case API_PHA:       // curved or sector array (for phased array apex is the virtual apex)
                elePerDeg = (float)(1/((180/PI)*(atan(pitch/(10.0F * apex)))));
                elePerLine = elePerDeg/density;
                break;
            case API_CLA:	    // curved or sector array (for phased array apex is the virtual apex)
                elePerDeg = (float)(1/((180/PI)*(atan(pitch/(10.0F * apex)))));
                elePerLine = elePerDeg/density;
                break;
            case API_DEFAULT:
                break;
        }
		numEles = roundf(elePerLine * numLines);
		maxSweepLength = numEles + apSize;
		for(apIndex=0; apIndex < maxSweepLength; apIndex++)
		{
			apArray[apIndex] = apIndex < apSize ? 1 : 0;  // An array representing the aperture
			apArraySlide[apIndex]=0;   //  A zero filled array the full length of the sweep
		}
			
		totalApPower = 0.0F;
		currentSlide = 0;
		int apIndexBStart = 0;
		for(apIndex=0; apIndex < numLines; apIndex++)  // simulation loop to slide aperture across the array face
		{
			for(int apIndexB = apIndexBStart; apIndexB < maxSweepLength; apIndexB++) //  creates the power profile as the aperture slides across the array   
			{
				apArraySlide[apIndexB] += apArray[apIndexB];
			}
				
			newSlide = roundf(apIndex * elePerLine);
			if (newSlide != currentSlide)
			{
				numElements2Slide = newSlide - currentSlide;
				currentSlide = newSlide;
				for(int slideIndex = 0; slideIndex < numElements2Slide; slideIndex++)
				{
					tempValue = apArray[maxSweepLength - 1];
					for(int apIndexB = maxSweepLength - 1; apIndexB > 0; apIndexB--)  //  Slides aperture one element to the right
						apArray[apIndexB] = apArray[apIndexB - 1];
					apArray[0] = tempValue;
				}
				apIndexBStart++;
			}
		}
		for(apIndex=0; apIndex < maxSweepLength; apIndex++)  // loop to sum the total power not accounting for aperture edge effects
			totalApPower += apArraySlide[apIndex];
		eles_per_cm = roundf(10.0F/pitch);
		availApPower = 0.0F;
		oneCmApPower = 0.0F;
		minIndex = roundf((maxSweepLength-maxElements)/2.0);
		maxIndex = roundf(maxSweepLength - ((int)(maxSweepLength - maxElements)/2 + (maxSweepLength - maxElements)%2)) - 1;
		minIndex_1_cm = roundf((maxSweepLength - eles_per_cm)/2);
		maxIndex_1_cm = roundf(maxSweepLength - ((int)(maxSweepLength - eles_per_cm)/2 +(maxSweepLength - eles_per_cm)%2)) - 1;
		for(apIndex=0; apIndex < maxSweepLength; apIndex++)	   // loop through power profile and sun power contributions for available and 1 cm power
		{
			availAp[apIndex] = (apIndex >= minIndex && apIndex <= maxIndex) ?  apArraySlide[apIndex] : 0; // create array of available power distribution across the array face
			availApPower += (apIndex >= minIndex && apIndex <= maxIndex) ?  apArraySlide[apIndex] : 0;  // sum the available power
			oneCmAp[apIndex] = (apIndex >= minIndex_1_cm && apIndex <= maxIndex_1_cm) ?  apArraySlide[apIndex] : 0;  // create array of 1 cm window power distribution 
			oneCmApPower += (apIndex >= minIndex_1_cm && apIndex <= maxIndex_1_cm) ?  apArraySlide[apIndex] : 0;  // sum the 1 cm power
		}
		pApiTable_utp->w0sFrac =  availApPower/totalApPower;   // Fraction of scanning power that accounts for edge effects as the aperture nears the edge of the array
		pApiTable_utp->w01sFrac = oneCmApPower/totalApPower;   // Fraction of scanning power within the center 1 cm window of the array
	}
}


/************************************************************************
* Function name:       GetScannedApertureArea
*
* Purpose:    Calculates and returns the scanned active aperture area in cm^2.
*
* Inputs:     API_TABLE_p_t pApiTable:  Pointer to API table
*             API_INVECTOR_UTP_p_t InputVector_utp: An input vector for a single UTP 
*             int utp_index:  Index into the UTP table portion of the UPS API table for
*                             the UTP being evaluated
*
* Returns      float  asaprt:  scanned active aperture area in cm^2
*                                                    
* Notes:
*                                                       
*
*************************************************************************/
float ApiCalculator::GetScannedApertureArea(API_TABLE_p_t pApiTable,
                              API_INVECTOR_UTP_p_t InputVector_utp,
                              int utp_index)
{
    float asaprt = 0.0F;
    float eleLength = pApiTable->api_table_header.EleLength;
    float crysToSkin = pApiTable->api_table_header.CrysToSkin;
    float pitch = pApiTable->api_table_header.ElePitch / 10.0F;
    int numEles = pApiTable->api_table_header.NumEles;
    int numLines = InputVector_utp->lpFrame;
    float density = InputVector_utp->density;
    float apex = pApiTable->api_table_header.Apex;
    API_XDUCER_CLASS xducerClass = pApiTable->api_table_header.Class;
    float yDim = 0.0F, yDimElements = 0.0F, yDim1 = 0.0F, yDim2 = 0.0F, zDim = 0.0F; 

    assert(eleLength != 0.0F);
    assert(pitch != 0.0F);
    assert(density != 0.0F);

	
    if(xducerClass == API_CLA || xducerClass == API_PHA)   /* check if sector type array  */   
    {
		if(apex == 0.0F)  // if the aperture doesn't slide, just use the reference unscanned aperture area
		{
			asaprt = pApiTable->api_utpid_table[utp_index]->aaprt_ref;
		}
		else
		{
			zDim = apex - crysToSkin;
			yDimElements = (float)roundf(((zDim * (PI/180 * (numLines - 1)/density)) +
                                    (pApiTable->api_utpid_table[utp_index]->aaprt_ref / eleLength))
                                    / pitch);
			yDim = pitch * (yDimElements < numEles ? yDimElements : numEles);
			asaprt = eleLength * yDim;
		}
    }     
    else            /* else its a linear array */
    {
        yDim1 = (numLines/(density * 10.0F)) + (pApiTable->api_utpid_table[utp_index]->aaprt_ref / eleLength);
        yDim2 = numEles * pitch;
        asaprt = eleLength * (yDim1 < yDim2 ? yDim1 : yDim2);
    }
    return (float) asaprt;
}
/************************************************************************
* Function name:       PolySolve
*
* Purpose:    This function is used to numerically solve the interpolating 
*			  polynomial for the transmit drive voltage that yields the
*			  search value.  Polynomial is solved by using the method of 
*			  successive bisections
*
* Inputs:     float coeffs[]:  Estimating polynomial coefficients
*			  float xMin:   Initial minimum value of the range over which the solution is
*						    is to be found.  Normally the minimum ref voltage.
*			  float xMax:   Initial maximum value of the range over which the solution is
*						    is to be found.  Normally the maximum ref voltage.
*			  float searchValue: Either the SPTA or MI value being sought.  Normally
*							 set at a control limit.
*			  float *actualValue:  Pointer to the actual value found once the tolerance
*							constraint is met
*			  float tol		Tolerance value for allowable error between the actual value
*							resulting from evaluation of the polynomial, and the value
*							being sought.
*			  float *error  Pointer to the error value 
*
* Returns  int  xMid:  x value that solves the polynomial
*                                                    
* Notes:
*
*************************************************************************/
float ApiCalculator::PolySolve(float coeffs[], float xMin, float xMax, float searchValue, float *actualValue, float tol, float *error)
{
    float xUpper = xMax, xMid = 0.0, xLower = xMin, rtnVal;

    *error = 999.0F;
    while (*error > tol)
    {
        xMid= (xUpper + xLower)/2.0F;
        *actualValue = CUBIC(coeffs[0], coeffs[1], coeffs[2], coeffs[3], xMid);
        if (*actualValue >= searchValue)
        {
            xUpper=xMid;
        }
        else
        {
            xLower=xMid;
        }
        *error = (float)fabs(*actualValue - searchValue);
    }
    rtnVal = xMid;

    return rtnVal;
}
/************************************************************************
* Function name:       GetMaxTI
*
* Purpose:    Finds the maximum thermal index (TI) from the TI values that
*             have been previously calculated and placed in the output vector.
*             This function is called by TI_VlimCalculate and is required for
*             calculating the drive voltage limitation due to thermal index.
*
* Inputs:     API_OUTVECTOR_p_t pApiOutvector   Pointer to the output vector
*
* Returns      float  maxTI:  Maximum thermal index
*
* Notes:
*
*
* Change History:
*   Date    Description
*  -------- -----------
*  03/20    Created
*
*************************************************************************/
float ApiCalculator::GetMaxTI(API_OUTVECTOR_p_t pApiOutvector)
{
    float maxTI = 0.0F;
    float tempMaxTI = -1000.0F;

    if (pApiOutvector->tisas > tempMaxTI)
    {
        tempMaxTI = pApiOutvector->tisas;
    }
    if (pApiOutvector->tisbs > tempMaxTI)
    {
        tempMaxTI = pApiOutvector->tisbs;
    }
    if (pApiOutvector->tibbs > tempMaxTI)
    {
        tempMaxTI = pApiOutvector->tibbs;
    }
    if (pApiOutvector->tic > tempMaxTI)
    {
        tempMaxTI = pApiOutvector->tic;
    }
    maxTI = tempMaxTI;

    return maxTI;
}

