//
//  AutoDGPModelAbstractor.h
//  EchoNous
//
//  Created by Michael May on 7/18/23.
//

#ifndef AutoDGPModelAbstractor_h
#define AutoDGPModelAbstractor_h
#include <stdint.h>
#include <vector>
class AutoDGPModelAbstractor
{
public:
    virtual ~AutoDGPModelAbstractor() {};
    virtual void init() = 0;
    virtual void processFrame(uint8_t* input, float** outputs) = 0;

};

#endif /* AutoDGPModelAbstractor_h */
