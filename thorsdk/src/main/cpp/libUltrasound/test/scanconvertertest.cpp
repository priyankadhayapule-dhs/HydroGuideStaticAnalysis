#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ScanConverterParams.h>
#include <ThorUtils.h>
#include "ScanConverterCl.h"
#include "ScanConverterReference.h"

#define TWO_PI 2*3.1415926f

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    int numSamples = 512;
    float startSampleMm = 0.0f;
    float sampleSpacingMm = 180.0f / (numSamples - 1);
    int numRays = 92;
    float raySpacing = TWO_PI / 360.0f;
    float startRayRadian = -(((float) numRays) / 2.0f - 0.5f) * raySpacing;

    int screenW, screenH;

    // thor ev
    screenW = 432;
    screenH = 614;

    ScanConverterParams sCParams = ScanConverterParams();

    sCParams.numRayMesh = 50;
    sCParams.numSampleMesh = 100;
    sCParams.numSamples = numSamples;
    sCParams.numRays = numRays;
    sCParams.startSampleMm = startSampleMm;
    sCParams.startRayRadian = startRayRadian;
    sCParams.sampleSpacingMm = sampleSpacingMm;
    sCParams.raySpacing = raySpacing;

    // allocate memory input, outputCL, outputRef;
    u_char* input_img =  new u_char[numSamples * numRays];

    u_char* output_CL = new u_char[screenW * screenH];
    u_char* output_Ref = new u_char[screenW * screenH];

    u_char* mask_in = new u_char[numSamples * numRays];
    u_char* mask_out = new u_char[screenW * screenH];


    // test pattern
    float ux, uy;

    for (int j = 0; j < numRays; j++) {
        for (int i = 0; i < numSamples; i++) {
            ux = (i+j)%256;
            uy = (i)%256;

            input_img[j*numSamples + i] = ux;
        }
    }

    printf("Running a scan converter test - B-mode!\n");
    printf("\n");

    ScanConverterCL scanConverterCL = ScanConverterCL(ScanConverterCL::OutputType::UCHAR8);
    scanConverterCL.setConversionParameters(sCParams.numSamples,
                                            sCParams.numRays,
                                            screenW,    // screenWidth
                                            screenH,    // screenHeight
                                            0.0f,    // start img depth
                                            (sCParams.sampleSpacingMm * (sCParams.numSamples - 1) + sCParams.startSampleMm),  //end img depth
                                            sCParams.startSampleMm,
                                            sCParams.sampleSpacingMm,
                                            sCParams.startRayRadian,
                                            sCParams.raySpacing,
                                            1.0f,
                                            1.0f);

    scanConverterCL.init();

    // running scanconverterCL
    scanConverterCL.runCLTex(input_img, output_CL);



    // scan converte refernece
    ScanConverterReference scanConverterReference = ScanConverterReference();
    scanConverterReference.setConversionParams(sCParams.numSamples,
                                               sCParams.numRays,
                                               screenW,    // screenWidth
                                               screenH,    // screenHeight
                                               0.0f,    // start img depth
                                               (sCParams.sampleSpacingMm * (sCParams.numSamples - 1) + sCParams.startSampleMm),  //end img depth
                                               sCParams.startSampleMm,
                                               sCParams.sampleSpacingMm,
                                               sCParams.startRayRadian,
                                               sCParams.raySpacing);

    // zoom pan params
    scanConverterReference.setZoomPanParams(1.0f, 0.0f, 0.0f);
    scanConverterReference.setFlipXFlipY(1.0f, 1.0f);

    // running refernece scan converter
    scanConverterReference.runConversionBiCubic4x4(input_img, output_Ref);


    // compare two outputs
    // mask generation
    for (int j = 0; j < numSamples * numRays; j++) {
        mask_in[j] = 120;
    }

    scanConverterReference.runConversionBiCubic4x4(mask_in, mask_out);

    int val_pix_cnt = 0;            //valid pixel count
    double err2_sum = 0.0f;

    // calculate PSNR
    for (int i = 0; i < screenW * screenH; i++) {
        if (mask_out[i] > 100) {
            // only when mask is valid
            val_pix_cnt++;
            err2_sum += (double)((output_Ref[i] - output_CL[i]) * (output_Ref[i] - output_CL[i]));
        }
    }

    double mse = err2_sum/((double) val_pix_cnt);
    double PSNR = 10*log10(255*255/mse);

    printf("Test: Comparing CL output with Reference output\n");
    printf("\n");
    printf("PSNR: %f, no. valid pixels: %d\n", PSNR, val_pix_cnt);
    printf("\n");
    printf("Scan converter test has been completed!\n");

    // clean up
    delete[] input_img;
    delete[] output_CL;
    delete[] output_Ref;
    delete[] mask_in;
    delete[] mask_out;

    scanConverterCL.cleanupCL();

    return(0);
}
