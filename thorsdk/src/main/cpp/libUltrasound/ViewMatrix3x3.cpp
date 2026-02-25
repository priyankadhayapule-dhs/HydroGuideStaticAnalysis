//
// Copyright 2017 EchoNous Inc.
//
#include <stdlib.h>
#include <string.h>
#include <ViewMatrix3x3.h>

void ViewMatrix3x3::setIdentityM(float *m) {
    memset((void*)m, 0, 9*sizeof(float));
    m[0] = m[4] = m[8] = 1.0f;
}

void ViewMatrix3x3::multiplyMV(float *m, const float *lhs, const float *rhsv) {
    float t[3];
    for (int j = 0; j < 3; j++) {
        float sum = 0.0f;
        for (int i = 0; i < 3; i++) {
            sum += lhs[I(j,i)]*rhsv[I(i,0)];
        }
        t[j] = sum;
    }

    memcpy(m, t, sizeof(t));
}

void ViewMatrix3x3::multiplyMM(float *m, const float *lhs, const float *rhs) {
    float t[9];
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            float sum = 0.0f;
            sum += lhs[I(j,0)]*rhs[I(0,i)];
            sum += lhs[I(j,1)]*rhs[I(1,i)];
            sum += lhs[I(j,2)]*rhs[I(2,i)];

            t[I(j,i)] = sum;
        }
    }

    memcpy(m, t, sizeof(t));
}

void ViewMatrix3x3::inverseM(float* m, const float* in) {
    float t[9];

    float a11, a12, a13, a21, a22, a23, a31, a32, a33;
    a11 = in[I(0,0)];
    a12 = in[I(0,1)];
    a13 = in[I(0,2)];
    a21 = in[I(1,0)];
    a22 = in[I(1,1)];
    a23 = in[I(1,2)];
    a31 = in[I(2,0)];
    a32 = in[I(2,1)];
    a33 = in[I(2,2)];

    float detA = a11*(a22*a33-a32*a23) + a21*(a32*a13-a12*a33) + a31*(a12*a23-a22*a13);
    float RdetA = 1.0f/detA;

    t[I(0,0)] = a22*a33 - a23*a32;
    t[I(1,0)] = a23*a31 - a21*a33;
    t[I(2,0)] = a21*a32 - a22*a31;
    t[I(0,1)] = a13*a32 - a12*a33;
    t[I(1,1)] = a11*a33 - a13*a31;
    t[I(2,1)] = a12*a31 - a11*a32;
    t[I(0,2)] = a12*a23 - a13*a22;
    t[I(1,2)] = a13*a21 - a11*a23;
    t[I(2,2)] = a11*a22 - a12*a21;

    for (int i = 0; i < 9; i++) {
        t[i] *= RdetA;
    }

    memcpy(m, t, sizeof(t));
}
