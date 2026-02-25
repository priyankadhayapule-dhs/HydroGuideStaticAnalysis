//
// Copyright 2019 EchoNous Inc.
//
//
#pragma once
#include "ApiData.h"


#define PI 3.141592654F
#define RADIANS_TO_DEGREES 57.29577951F  /* conversion factor */
#define TIC_DENOM_CONST 45.1351667F  /* 40 x sqrt(4/pi)  */


//  Cubic polynomial macro used for polynomial interpolation estimation of MI and SPTA
#define CUBIC(A, B, C, D, x)  (((A)*(x)*(x)*(x)) + ((B)*(x)*(x)) + ((C)*(x)) + D)

class ApiCalculator
{
private:
    float Pii0Calculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float Pii0Calculate(API_TABLE_p_t, API_INVECTOR_p_t); 
  	float Pii3Calculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float Pii3Calculate(API_TABLE_p_t, API_INVECTOR_p_t); 
    float Sii3Calculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float Sii3Calculate(API_TABLE_p_t, API_INVECTOR_p_t);
	float SPPA3Calculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float SPPA3Calculate(API_TABLE_p_t, API_INVECTOR_p_t); 
	float SPTA3ScannedCalculate_utp (API_TABLE_p_t, API_INVECTOR_UTP_p_t, float);
	float SPTA3UnscannedCalculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float SPTA3Calculate(API_TABLE_p_t, API_INVECTOR_p_t); 
    float Imax3Calculate(API_TABLE_p_t, API_INVECTOR_p_t); 
    float Imax3Calculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float MICalculate(API_TABLE_p_t, API_INVECTOR_p_t); 
    float MICalculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float PowerUnscannedCalculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float PowerScannedCalculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float, int, int); 
    void AcousticPowerCalculate (API_TABLE_p_t, API_INVECTOR_p_t, float *, float *);
	float TISasCalculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float TISasCalculate(API_TABLE_p_t, API_INVECTOR_p_t); 
    float TISbsUnscannedCalculate_utp (API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float TISbsCalculate (API_TABLE_p_t, API_INVECTOR_p_t);
    float TIBbsUnscannedCalculate_utp(API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float TIBbsCalculate (API_TABLE_p_t, API_INVECTOR_p_t);
	float TICasScannedCalculate_utp (API_TABLE_p_t, API_INVECTOR_UTP_p_t, float);
    float TICasUnscannedCalculate_utp (API_TABLE_p_t, API_INVECTOR_UTP_p_t, float); 
    float TIcCalculate (API_TABLE_p_t, API_INVECTOR_p_t);
    float SPTA_VlimCalculate(API_INVECTOR_p_t, API_TABLE_p_t, API_OUTVECTOR_p_t); 
    float MI_VlimCalculate(API_INVECTOR_p_t, API_TABLE_p_t, API_OUTVECTOR_p_t);
    float TI_VlimCalculate(API_INVECTOR_p_t, API_TABLE_p_t, API_OUTVECTOR_p_t);
	float PolySolve(float[], float, float, float, float *, float, float *);
    float GetMinVdrv(API_OUTVECTOR_p_t);
	API_IMAGE_MODE GetImageMode(API_INVECTOR_p_t);
	void GetMaskFractions(API_INVECTOR_UTP_p_t, API_TABLE_p_t, API_UTPID_TABLE_p_t);
   	float GetScannedApertureArea(API_TABLE_p_t, API_INVECTOR_UTP_p_t, int);
	float GetVdrvSolution(float[], float, float, float, float, API_EXTRAP_METHOD);
    float GetMaxTI(API_OUTVECTOR_p_t);
public:
	ApiCalculator() {};
    ~ApiCalculator() {};
    		
    void CalculateOutputVector (API_INVECTOR_p_t, API_TABLE_p_t, API_OUTVECTOR_p_t, int);
    float GetCommandVdrv(API_INVECTOR_p_t, API_TABLE_p_t, API_OUTVECTOR_p_t);
};

