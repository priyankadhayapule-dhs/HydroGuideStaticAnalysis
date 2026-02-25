//
// Created by Eunghun Kim on 08/23/17.
//

#pragma once
#ifndef PI
#define PI 3.1415926f
#endif
#define I(_j, _i) ((_j)+3*(_i))

// 3x3 matrix/vector operations needed to calculate model view matrix
namespace ViewMatrix3x3
{
    void setIdentityM(float *m);
    void multiplyMV(float *m, float *lhs, float *rhsv);
    void multiplyMM(float *m, float *lhs, float *rhs);
    void inverseM(float* m, float* in);
}