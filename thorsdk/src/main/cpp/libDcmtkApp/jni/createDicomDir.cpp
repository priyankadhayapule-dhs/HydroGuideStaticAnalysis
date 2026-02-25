//
// Created by himanshumistri on 12/2/21.
//

#include <jni.h>


#include "include/dcmtkClientJni.h"
#include "include/scu.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <android/log.h>

#define DEFAULT_FILESETID "ECHONOUS_MEDIA"

#include "dcmtk/dcmdata/dcddirif.h"    /* for class DicomDirInterface */
#include "dcmtk/ofstd/ofstd.h"         /* for class OFStandard */

#include "dcmtk/dcmimage/diregist.h"   /* include to support color images */
#include "dcmtk/dcmdata/dcrledrg.h"    /* for DcmRLEDecoderRegistration */
#include "dcmtk/dcmjpeg/djdecode.h"    /* for dcmjpeg decoders */
#include "dcmtk/dcmjpeg/dipijpeg.h"    /* for dcmimage JPEG plugin */
#include "dcmtk/dcmjpeg/ddpiimpl.h"    /* for class DicomDirImageImplementation */
using namespace std;

#define OFFIS_CONSOLE_APPLICATION "ECHONOUSAPP"

#define LOGIDIR(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))





JNIEXPORT jobject
DCMTKCLIENT_METHOD(createDicomDir)(JNIEnv* env, jobject thiz,jstring inputDir,jstring outputDir,jstring writeAction) {

    static OFLogger dcmgpdirLogger = OFLog::getLogger("dcmtk.dcmdata." OFFIS_CONSOLE_APPLICATION);
    DcmRLEDecoderRegistration::registerCodecs();
    DJDecoderRegistration::registerCodecs();

    OFBool opt_write = OFTrue;
    OFBool opt_append = OFFalse;
    OFBool opt_update = OFFalse;
    OFBool opt_recurse = OFTrue;

    E_EncodingType opt_enctype = EET_ExplicitLength;
    E_GrpLenEncoding opt_glenc = EGL_withoutGL;

    const char* mMainDir=env->GetStringUTFChars((jstring)inputDir, nullptr);
    const char* mOutputDirectory=env->GetStringUTFChars((jstring)outputDir, nullptr);
    const char* mActionWrite=env->GetStringUTFChars((jstring)writeAction, nullptr);
    LOGIDIR("DICOMDIR:: Action Write Type1 %s",mActionWrite);
    string action(mActionWrite);
    LOGIDIR("DICOMDIR:: Action Write Type2 %s",action.c_str());

    string mInputDir(mMainDir);
    mInputDir.append("DICOM");

    OFFilename inputDirectory(mInputDir);
    OFFilename inputRoot(mMainDir);

    OFFilename opt_pattern;

    OFFilename opt_output(mOutputDirectory);

    const char *opt_fileset = DEFAULT_FILESETID;
    const char *opt_descriptor = nullptr;
    const char *opt_charset = DEFAULT_DESCRIPTOR_CHARSET;

    DicomDirInterface::E_ApplicationProfile opt_profile = DicomDirInterface::AP_GeneralPurposeDVDJPEG;
    /* DICOMDIR interface (checks for JPEG/RLE availability) */
    DicomDirInterface ddir;
    opt_profile = DicomDirInterface::AP_GeneralPurposeDVDJPEG;


    if ( action == "append")
    {
        LOGIDIR("DICOMDIR:: append found %s", action.c_str());
        opt_write = OFTrue;
        opt_append = OFTrue;
        opt_update = OFFalse;
    }else{
        LOGIDIR("DICOMDIR:: new creation found %s", action.c_str());
    }

    ddir.enableMapFilenamesMode(OFFalse);
    ddir.enableAbortMode(OFTrue);
    ddir.disableFileFormatCheck(OFFalse);
    ddir.disableConsistencyCheck(OFFalse);
    /* create list of input files */
    OFFilename paramValue;
    OFFilename pathName;
    OFList<OFFilename> fileNames;
    //OFString pathname;
    const int count = 1;
    /* make sure input directory exists (if specified) */
    if (!inputDirectory.isEmpty())
    {
        if (!OFStandard::dirExists(inputDirectory))
        {
            //OFLOG_FATAL(dcmgpdirLogger, OFFIS_CONSOLE_APPLICATION << ": specified input directory does not exist");
            LOGIDIR("ERROR:: Input Directory not found");
            //return 1;
        }
    }



    /* iterate over all input filenames */
    for (int i = 1; i <= count; i++)
    {
        pathName.set(mInputDir);

        //cmd.getParam(i, param);
        /* add input directory */
        OFStandard::combineDirAndFilename(pathName, inputDirectory, paramValue, OFTrue /*allowEmptyDirName*/);
        //paramValue.set("PATIENT");
        LOGIDIR("GOOD:: PATH Name is %s", pathName.getCharPointer());
        /* search directory recursively (if required) */
        if (OFStandard::dirExists(pathName))
            OFStandard::searchDirectoryRecursively(paramValue, fileNames, opt_pattern, inputDirectory);
        else
            fileNames.push_back(paramValue);
    }


    /* check whether there are any input files */
    if (fileNames.empty())
    {
        LOGIDIR("ERROR:: no input files: DICOMDIR not created");
    }else{
        LOGIDIR("GOOD:: File input counts %d", fileNames.size());
    }

    // add image support to DICOMDIR class
    DicomDirImageImplementation imagePlugin;
    ddir.addImageSupport(&imagePlugin);

    OFCondition result;

    /* create new general purpose DICOMDIR, append to or update existing one */
    if (opt_append)
    {
        action = "appending";
        result = ddir.appendToDicomDir(opt_profile, opt_output);
        LOGIDIR("GOOD:: DICOMDIR:: Append %s", result.text());
    }
    else {
        action = "creating";
        result = ddir.createNewDicomDir(opt_profile, opt_output, opt_fileset);
        LOGIDIR("GOOD:: DICOMDIR:: Create %s", result.text());
    }


    if (result.good())
    {
        /* set fileset descriptor and character set */
        result = ddir.setFilesetDescriptor(opt_descriptor, opt_charset);

        if (result.good()) {
            /* collect 'bad' files */
            OFList<OFFilename> badFiles;
            unsigned int goodFiles = 0;
            auto iter = fileNames.begin();
            auto last = fileNames.end();
            /* iterate over all input filenames */
            while ((iter != last) && result.good())
            {
                LOGIDIR("GOOD:: Add dicom file name is %s", iter->getCharPointer());
                /* add files to the DICOMDIR */
                const char *fileName= iter->getCharPointer();

                string mFinalString("DICOM/");
                mFinalString.append(fileName);
                LOGIDIR("GOOD:: Add dicom filename is %s", mFinalString.c_str());

                //(*iter).set("",OFFalse);
                OFFilename dicomFile(mFinalString);

                result = ddir.addDicomFile(dicomFile, inputRoot);
                if (result.bad())
                {
                    LOGIDIR("ERROR:: DICOM add file %s", result.text());
                    badFiles.push_back(*iter);
                    if (!ddir.abortMode())
                    {
                        /* ignore inconsistent file, just warn (already done inside "ddir") */
                        result = EC_Normal;
                    }
                } else
                    ++goodFiles;
                ++iter;
            }

            /* evaluate result of file checking/adding procedure */
            if (goodFiles == 0)
            {
                LOGIDIR("ERROR:: no good files: DICOMDIR not created");
                result = EC_IllegalCall;
            }
            else if (!badFiles.empty())
            {
                LOGIDIR("ERROR:: file(s) cannot be added to DICOMDIR:");

                OFOStringStream oss;
                oss << badFiles.size() << " file(s) cannot be added to DICOMDIR: ";
                iter = badFiles.begin();
                last = badFiles.end();
                while (iter != last)
                {
                    oss << OFendl << "  " << (*iter);
                    ++iter;
                }
                oss << OFStringStream_ends;
                OFSTRINGSTREAM_GETSTR(oss, tmpString)
                    OFLOG_WARN(dcmgpdirLogger, tmpString);
                OFSTRINGSTREAM_FREESTR(tmpString)
            }

            /* write DICOMDIR file */
            if (result.good() && opt_write)
            {
                LOGIDIR("GOOD::  DICOMDIR  created");
                action = "writing";
                result = ddir.writeDicomDir(opt_enctype, opt_glenc);
            }
        }

    }

    /* some final error reporting */
    if (result.bad() && (result != EC_IllegalCall))
    {
        LOGIDIR("ERROR:: DICOMDIR  Error %s", result.text());
        OFLOG_FATAL(dcmgpdirLogger, OFFIS_CONSOLE_APPLICATION << ": error ("
                                                              << result.text() << ") " << action << " file: " << opt_output);
    }

    DcmRLEDecoderRegistration::cleanup();
    DJDecoderRegistration::cleanup();

    return getResultObject(env,"Success",SUCCESS);
}