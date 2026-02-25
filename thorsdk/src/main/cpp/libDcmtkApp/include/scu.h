//
// Created by echonous on 14/6/18.
//

#ifndef DCMTKCLIENT_TESTSCU_H
#define DCMTKCLIENT_TESTSCU_H

#include "dcmtk/config/osconfig.h"  /* make sure OS specific configuration is included first */
#include "dcmtk/dcmnet/scu.h"     /* Covers most common dcmdata classes */
#include "dcmtk/dcmtls/tlslayer.h"
#include <android/log.h>

#define ERROR 0
#define SUCCESS 1

#define LOGI(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))
#define LOGE(...) \
((void)__android_log_print(ANDROID_LOG_ERROR, "dcmtk::", __VA_ARGS__))

class DcmClient : public DcmSCU
{
private:
    DcmTLSTransportLayer* tLayer = nullptr;

public:
    DcmClient()  {

    }
    ~DcmClient() {

    }

    OFCondition setTransportLayer(const char *pemTlsCertificatePath, bool useClientAuthentication) {
        DcmTLSTransportLayer::initializeOpenSSL();
        tLayer = new DcmTLSTransportLayer(NET_REQUESTOR, "/dev/random", OFFalse);

        OFCondition result;
        if ((bool)useClientAuthentication) {
            tLayer->addTrustedCertificateFile(pemTlsCertificatePath, DCF_Filetype_PEM);
            result =tLayer->setPrivateKeyFile(pemTlsCertificatePath, DCF_Filetype_PEM);
            if (result.bad())
            {
                string errorMessage;
                errorMessage.append("Unable to load private key : ");
                errorMessage.append(result.text());
                LOGE("%s",errorMessage.c_str());
                return result;
            } else {
                LOGI("Key loaded successfully: %s", result.text());
            }

            result = tLayer->setCertificateFile(pemTlsCertificatePath, DCF_Filetype_PEM, TSP_Profile_BCP_195_RFC_8996);
            if (result.bad())
            {
                string errorMessage;
                errorMessage.append("Unable to load Certificate File : ");
                errorMessage.append(result.text());
                LOGE("%s",errorMessage.c_str());
                return result;
            } else {
                LOGI("Certificate loaded successfully: %s", result.text());
            }
            tLayer->setCertificateVerification(DCV_requireCertificate);
        } else{
            tLayer->setCertificateVerification(DCV_ignoreCertificate);
        }

        tLayer->setTLSProfile(TSP_Profile_BCP_195_RFC_8996);
        tLayer->activateCipherSuites();
        result = this->useSecureConnection(tLayer);
        if (result.bad()) {
            string errorMessage;
            errorMessage.append("Unable to use tls : ");
            errorMessage.append(result.text());
            LOGE("%s",errorMessage.c_str());
            return result;
        } else {
            LOGI("TLS applied successfully: %s", result.text());
        }
        return result;
    }

    void clearTransportLayer() {
        if (nullptr != tLayer) {
            tLayer->clear();
            tLayer = nullptr;
        }
    }
};


#endif //DCMTKCLIENT_TESTSCU_H
