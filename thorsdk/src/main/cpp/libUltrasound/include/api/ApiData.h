//
// Copyright 2019 EchoNous Inc.
//
//
#pragma once


#define API_MAXUTPTABLESIZE 500 /* Max number of UTP IDs in the API data table  */
#define API_MAXUTPSIZE 10
#define API_MAXNUMUTPS 3
#define API_FALSE   (0)
#define API_TRUE    (1)


typedef enum  /*  Operating Modes  */
{
    API_Es,      /*  Echo, scanned  */
    API_Eu,      /*  Echo, unscanned  */ 
    API_Ps,      /*  Pulsed, scanned (i.e. CPD or DCPD)  */ 
    API_Pu,      /*  Pulsed, unscanned (i.e. PWD)  */ 
    API_Cu      /*  Continuous Wave (i.e. CW)  */
} API_PULSE_MODE;

typedef enum  /*  Transducer class  */
{
    API_CLA,     /*  Curved  */
    API_LA,      /*   Linear  */
    API_PHA,     /*   Phased  */
    API_DEFAULT
} API_XDUCER_CLASS;

typedef enum  /*  Extrapolation methods  */
{
    API_QUAD,    /*  Quadratic  */
    API_LIN      /*   Linear  */
} API_EXTRAP_METHOD;

typedef enum /* Thermal index types */
{
    TISAS,
    TISBS,
    TIBBS,
    TIC,
    API_TI_TYPE_DEFAULT
} API_TI_TYPE;


typedef enum  /*  Imaging Modes  */
{
    B_MODE,      //  B Mode
    M_MODE,      //  M Mode
    COLOR,       //  Color or CPD
    PWD,        //  PW Doppler
    CW          //  CW Doppler
} API_IMAGE_MODE;



/***  structure for a single UTP of an input vector  ***/
typedef struct api_inVector_utp_s
{
    int            utp;
    float          prf;
    float          fr;
    int            ppl;
    float          density;
    int            lpFrame;
    API_PULSE_MODE   pulsMode;
} API_INVECTOR_UTP_t, *API_INVECTOR_UTP_p_t;

/***  structure for input vector  ***/
typedef struct api_inVector_s
{
    API_INVECTOR_UTP_t  api_invector_utp[API_MAXNUMUTPS];
    float               txVdrv;    // The transmit drive voltage 
    int                 numUtps;    // Number of UTPs in the input vector
} API_INVECTOR_t, *API_INVECTOR_p_t;


// This structure is used for the EchoNous Thor release
typedef struct api_utpIDTable_s
{
    int    utpID;
	int	   apEles;
	float   ld_ref;
	float   aaprt_ref;
	float   cFreq;
	float   w01sFrac;
	float   w0sFrac;
	float   pii_0_ref;
	float   pii_3_ref;
	float   sii_3_ref;
	float   sppa_3_ref;
	float   mi_3_ref;
	float   imax_3_ref;
	float   vdrv_ref;
	float   w0u_ref;
	float	vdrv_corr;
	float   TISas_ref;
	float   TISbs_ref;
	float   TIBas_ref;
	float   TIBbs_ref;
	float   min_ref_vdrv;
    float   max_ref_vdrv;
	float	pr_3;
    float   mi_coeff_A;
    float   mi_coeff_B;
    float   mi_coeff_C;
    float   pii3_coeff_A;
    float   pii3_coeff_B;
    float   pii3_coeff_C;
    float   pii3_coeff_D;
    float   sii3_coeff_A;
    float   sii3_coeff_B;
    float   sii3_coeff_C;
    float   sii3_coeff_D;
}API_UTPID_TABLE_t, *API_UTPID_TABLE_p_t;

/***  structure for contents of API data table header  ***/
typedef struct api_table_header_s
{
    float	SPTA_lim;
    float	MI_lim;
    float	TI_lim;
    float	ambient;
    float	Apex;
    float	ElePitch;
    float	CrysToSkin;
    float	EleLength;
    int		NumEles;
    API_XDUCER_CLASS    Class;
} API_TABLE_HEADER_t, *API_TABLE_HEADER_p_t;

/***  structure for API data table  ***/

typedef struct api_table_s
{
    API_TABLE_HEADER_t	api_table_header;
    API_UTPID_TABLE_t	*api_utpid_table[API_MAXUTPTABLESIZE];
} API_TABLE_t, *API_TABLE_p_t;


/***  structure for output vector  ***/
typedef struct api_outVector_s
{
    float   pii_0;                                                     
    float   pii_3;
    float   sppa_3;			
    float   spta_3;
    float   mi;
    float   imax_3;
    float   w0;
    float   w01;
    float   tisas;
    float   tisbs;
    float   tibbs;
    float   tic;
    float   tis_display;
    float   tib_display;
    float   tic_display;
    float   spta_vlim;
    float   mi_vlim;
    float   ti_vlim;
	float   vdrvCmd;
} API_OUTVECTOR_t, *API_OUTVECTOR_p_t;
