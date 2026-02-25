//
// Created by Echonous on 18/10/19.
//

#ifndef KOSMOSAPP_DJENCODER_H
#define KOSMOSAPP_DJENCODER_H
#include "dcmtk/config/osconfig.h"  /* make sure OS specific configuration is included first */
#include <dcmtk/dcmjpeg/djcodece.h>
#include <dcmtk/dcmjpeg/djencbas.h>
#include <dcmtk/dcmjpeg/djrploss.h>
#include <dcmtk/dcmjpeg/djeijg8.h>

class DcmEncoder: public DJCodecEncoder{

        public:
        DcmEncoder()  {

        }
        ~DcmEncoder() {

        }


};

#endif //KOSMOSAPP_DJENCODER_H
