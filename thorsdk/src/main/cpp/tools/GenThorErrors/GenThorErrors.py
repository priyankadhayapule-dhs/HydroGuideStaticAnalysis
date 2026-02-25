#
# Copyright 2017 EchoNous Inc.
#

import json
import sys
import os

# This is the input JSON file used to generate the C/C++ Header and the Java
# class file.
JSON_FILE = './ThorError.json'

# Relative path to root of apps directory
SOURCE_ROOT = '../../..'

# This is the output C/C++ Header file
C_HEADER_FILE = '/cpp/libThorCommon/include/ThorError.h'

# This is the output Java class file
JAVA_CLASS_FILE = '/java/com/echonous/hardware/ultrasound/ThorError.java'

# This is the file used to generate the pre-canned portion of the Java class
JAVA_BOILERPLATE_FILE = './ThorError-Boilerplate.java'

CSV_FILE = "./ThorError.csv"

THOR_ERROR = 0x40000000

class TextColors:
    SUCCESS = '\033[92m'
    FAIL = '\033[91m'

#------------------------------------------------------------------------------
def usage():
    print('usage: python3 GenThorErrors.py PATH')
    print('\n  PATH\tPath to JSON file containing Thor error definitions.\n')

#------------------------------------------------------------------------------
# Dump contents of file to screen
#------------------------------------------------------------------------------
def printFile(pathname):
    fd = open(pathname, 'r')
    jsonData = fd.read()
    print(jsonData)


#------------------------------------------------------------------------------
def genCopyright():
    print('//\n// Copyright 2017 EchoNous Inc.')
    print('//\n// Thor specific error codes')
    print('//\n// This file was automatically generated.  Do not edit!\n//')

#------------------------------------------------------------------------------
def genDescription():
    print('//\n//  31                27              15              0')
    print('//  ---------------------------------------------------')
    print('//  | x | ERR | x | x |      TES      |     Code      |')
    print('//  ---------------------------------------------------')
    print('//')
    print('//  Label     Bit(s)         Description')
    print('//  -----------------------------------------------------------')
    print('//   x      28, 29, 31   Reserved for future use')
    print('//   ERR        30       Error bit.  1 = error, 0 = success')
    print('//   TES     16 - 27     Thor Error Source')
    print('//   Code    0 - 15      Error Code\n//\n')

#------------------------------------------------------------------------------
# Verify that the file can be opened and that it is a valid JSON file.
# Ensure that supplied values for Error Sources and Errors are unique.  Only
# checks "label" and "value" for uniqueness.
#------------------------------------------------------------------------------
def validateJson(inputJsonFile):
    try:
        fd = open(inputJsonFile, 'r')
        jsonObj = json.load(fd)
    except IOError:
        print(TextColors.FAIL + 'Unable to open JSON file: %(jsonFile)s' % {'jsonFile': inputJsonFile})
        return False
    except ValueError as jsonException:
        print(TextColors.FAIL + 'The JSON file has invalid format: %(jsonFile)s' % {'jsonFile': inputJsonFile})
        print(jsonException.args)
        return False

    errorSources = set()

    # This maintains uniqueness for both Error Source labels and Error labels
    errorLabels = set()

    #
    # Verify all error sources
    #
    for errorSource in jsonObj['ErrorSources']:
        errorSourceValue = int(errorSource['value'], 16)

        # 0 is not a valid source value
        if 0 == errorSourceValue:
            print(TextColors.FAIL + 'The JSON input file is invalid')
            print('Error source value for %(errorSourceLabel)s cannot be zero' % 
                  {'errorSourceLabel': errorSource['label']
                   })
            return False

        # The source value must be unique
        if errorSourceValue in errorSources:
            print(TextColors.FAIL + 'The JSON input file is invalid')
            print('Error source value of 0x%(errorSourceVal)X for %(errorSourceLabel)s already exists' % 
                  {'errorSourceVal': errorSourceValue,
                   'errorSourceLabel': errorSource['label']
                   })
            return False

        errorSources.add(errorSourceValue)

        # The source label must be globally unique
        if errorSource['label'] in errorLabels:
            print(TextColors.FAIL + 'The JSON input file is invalid')
            print('Error source %(errorSourceLabel)s already exists' % 
                   {'errorSourceLabel': errorSource['label']
                   })
            return False

        errorLabels.add(errorSource['label'])

        # OK to have an error source with no errors defined
        if not 'errors' in errorSource:
            continue

        errors = set()

        #
        # Verify all errors for this source
        #
        for errorDef in errorSource['errors']:
            errorValue = int(errorDef['value'], 16)

            # 0 is not a valid error value
            if 0 == errorValue:
                print(TextColors.FAIL + 'The JSON input file is invalid')
                print('Error  value for %(errorLabel)s cannot be zero' % 
                      {'errorLabel': errorDef['label']
                       })
                return False

            # The error value must be unique
            if errorValue in errors:
                print(TextColors.FAIL + 'The JSON input file is invalid')
                print('Error value of 0x%(errorVal)X for %(errorLabel)s already exists' % 
                      {'errorVal': errorValue,
                       'errorLabel': errorDef['label'],
                       })
                return False

            errors.add(errorValue)

            # The error label must be globally unique
            if errorDef['label'] in errorLabels:
                print(TextColors.FAIL + 'The JSON input file is invalid')
                print('Error %(errorLabel)s already exists' % 
                       {'errorLabel': errorDef['label']
                       })
                return False

            # Commas are not allowed in name, description and cause
            # because it complicates generation of CSV file.
            cause = errorDef.get('cause', '')

            if errorDef['name'].find(',') != -1:
                print(TextColors.FAIL + 'No commas allowed in name field!')
                return False
            if errorDef['description'].find(',') != -1:
                print(TextColors.FAIL + 'No commas allowed in description field!')
                return False
            if cause.find(',') != -1:
                print(TextColors.FAIL + 'No commas allowed in cause field!')
                return False

            errorLabels.add(errorDef['label'])

    return True

#------------------------------------------------------------------------------
# Generate a C-style header file from JSON input.  If outputHeaderFile is not 
# passed then will print to screen instead of creating the file.
#------------------------------------------------------------------------------
def genHeaderFile(inputJsonFile, outputHeaderFile = None):
    fd = open(inputJsonFile, 'r')
    jsonObj = json.load(fd)

    # Redirecting stdout to the destination file allows use of print() and
    # automatic addition of '\n' to each line.  Also allows for debugging
    # by printing to stdout instead.
    if outputHeaderFile is not None:
        try:
            fd = open(SOURCE_ROOT + outputHeaderFile, 'w')
        except IOError:
            print(TextColors.FAIL + 'Unable to create file %(srcRoot)s%(outFile)s' % 
                  {'srcRoot' : SOURCE_ROOT, 'outFile': outputHeaderFile})
            return False

        sys.stdout = fd

    genCopyright()

    print('#pragma once\n\n')
    print('#include <stdint.h>\n\n')

    genDescription()

    print('//\n// Common Defines\n//\n')
    print('typedef uint32_t ThorStatus;\n')
    print('#define THOR_OK             0x0')
    print('#define THOR_ERROR          0x%(value)X' % {'value': THOR_ERROR})
    print('#define IS_THOR_ERROR(x)    (x & (THOR_ERROR))\n')
    print('#define DEFINE_THOR_ERROR(source, code) (THOR_ERROR | (source << 16) | code)')
    print('')

    print('//\n// Thor Error Sources (TES)\n//\n')

    # Generate Error Sources
    for errorSource in jsonObj['ErrorSources']:
        print('// %(name)s: %(description)s' % 
              {'name': errorSource['name'],
               'description': errorSource['description']})
        print('#define %(label)s\t\t%(value)s\n' % 
              {'label': errorSource['label'],
               'value': errorSource['value']})

    print('//\n// Thor Error Definitions\n//\n')

    # Generate Error Codes
    for errorSource in jsonObj['ErrorSources']:
        if not 'errors' in errorSource:
            continue

        print('//\n// %(name)s\n//' % {'name': errorSource['name']})

        for errorDef in errorSource['errors']:
            print('// %(name)s: %(description)s' % 
                  {'name': errorDef['name'],
                   'description': errorDef['description']})

            isError = errorDef.get('isError', True)
            if isError:
                value = THOR_ERROR | ( int(errorSource['value'], 16) << 16) | int(errorDef['value'], 16)
            else:
                value = ( int(errorSource['value'], 16) << 16) | int(errorDef['value'], 16)

            print('#define %(label)s\t\t0x%(value)X' %
                  {'label': errorDef['label'],
                   'value': value})
        print('')

    if outputHeaderFile is not None:
        sys.stdout = sys.__stdout__
        print('Header file generated: %(srcRoot)s%(outFile)s' % 
              {'srcRoot' : SOURCE_ROOT, 'outFile': outputHeaderFile} )

    return True

#------------------------------------------------------------------------------
# Generate a Java class file from JSON input.  If outputJavaFile is not 
# passed then will print to screen instead of creating the file.
#------------------------------------------------------------------------------
def genJavaFile(inputJsonFile, outputJavaFile = None):
    fd = open(inputJsonFile, 'r')
    jsonObj = json.load(fd)

    # Redirecting stdout to the destination file allows use of print() and
    # automatic addition of '\n' to each line.  Also allows for debugging
    # by printing to stdout instead.
    if outputJavaFile is not None:
        try:
            fd = open(SOURCE_ROOT + outputJavaFile, 'w')
        except IOError:
            #print(TextColors.FAIL + 'Unable to create file $ANDROID_BUILD_TOP%(outFile)s' % {'outFile': outputJavaFile})
            print(TextColors.FAIL + 'Unable to create file %(srcRoot)s%(outFile)s' % 
                  {'srcRoot' : SOURCE_ROOT, 'outFile': outputJavaFile})
            return False

        sys.stdout = fd

    genCopyright()

    print('package com.echonous.hardware.ultrasound;\n')
    print('import java.util.HashMap;')
    print('import android.content.Context;\n')

    print('/**')
    print(' * <code>ThorError</code> is an enumeration of Thor Error Sources and their Errors.')
    print(' * An Error Source is way of grouping errors from a common origin,')
    print(' * i.e., software component or hardware device.\n * <p>')
    print(' * Error Sources begin with <code>TES_</code> and Errors begin')
    print(' * with <code>TER_</code>.\n * <p>')
    print(' * All Error Sources and Errors are defined as unsigned integers')
    print(' * that are unique and encoded.  They are typically displayed as hexadecimal values.')
    print(' * For each error, there are methods for obtaining the raw code, ')
    print(' * name, description, and source.\n * <p>')
    print(' * There are two special or uncategorized Errors: {@link ThorError#THOR_ERROR}')
    print(' * and {@link ThorError#THOR_OK}.\n * <p>')
    print(' * <code>THOR_ERROR</code> is a generic error code.\n * <p>')
    print(' * <code>THOR_OK</code> is a success code.  However it is best to use')
    print(' * the {@link ThorError#isOK()} method')
    print(' * in case additional success codes are added in the future.\n * <p>')
    print(' */')

    print('public enum ThorError {')

    # Generate Generic Values
    print('    //\n    // Generic Values\n    //')
    print('    /**\n     * Operation successful\n     */')
    print('    THOR_OK(0x0, "OK", "Operation successful"),\n')
    print('    /**\n     * Unspecified or uncategorized error\n     */')
    print('    THOR_ERROR(0x%(value)X, "Error", "Unspecified or uncategorized error"),\n' % {'value': THOR_ERROR})

    # Generate Error Sources
    print('    //\n    // Thor Error Sources (TES)\n    //')
    for errorSource in jsonObj['ErrorSources']:
        print('    /** \n     * %(description)s\n     */' % 
              {'name': errorSource['name'],
               'description': errorSource['description']})
        print('    %(label)s(%(value)s, "%(name)s", "%(description)s"),\n' % 
              {'label': errorSource['label'],
               'value': errorSource['value'],
               'name': errorSource['name'],
               'description': errorSource['description']})

    print('    //\n    // Thor Error Definitions\n    //\n')

    # Find out how many Error Codes there are
    defCount = 0
    for errorSource in jsonObj['ErrorSources']:
        if not 'errors' in errorSource:
            continue
        for errorDef in errorSource['errors']:
            defCount += 1

    # Generate Error Codes
    iter = 0
    for errorSource in jsonObj['ErrorSources']:
        if not 'errors' in errorSource:
            continue

        print('    //\n    // %(name)s\n    //' % {'name': errorSource['name']})

        for errorDef in errorSource['errors']:
            iter += 1
            print('    /**\n     * %(description)s\n     */' % 
                  {'name': errorDef['name'],
                   'description': errorDef['description']})

            isError = errorDef.get('isError', True)
            if isError:
                value = THOR_ERROR | ( int(errorSource['value'], 16) << 16) | int(errorDef['value'], 16)
            else:
                value = ( int(errorSource['value'], 16) << 16) | int(errorDef['value'], 16)

            if iter < defCount:
                print('    %(label)s(0x%(value)X, "%(name)s", "%(description)s"),\n' % 
                      {'label': errorDef['label'],
                       'value': value,
                       'name': errorDef['name'],
                       'description': errorDef['description']})
            else:
                # Last one ends in ';' instead of ','
                print('    %(label)s(0x%(value)X, "%(name)s", "%(description)s");\n' % 
                      {'label': errorDef['label'],
                       'value': value,
                       'name': errorDef['name'],
                       'description': errorDef['description']})

    # Generate standard Java code for this class
    boilerplateFd = open(JAVA_BOILERPLATE_FILE, 'r')
    print (boilerplateFd.read())

    print('}')

    if outputJavaFile is not None:
        sys.stdout = sys.__stdout__
        print('Java class file generated: %(srcRoot)s%(outFile)s' % 
              {'srcRoot' : SOURCE_ROOT, 'outFile': outputJavaFile} )

    return True

#------------------------------------------------------------------------------
# Generate a CSV file from JSON input.  If outputCsvFile is not 
# passed then will print to screen instead of creating the file.
#------------------------------------------------------------------------------
def genCsvFile(inputJsonFile, outputCsvFile = None):
    fd = open(inputJsonFile, 'r')
    jsonObj = json.load(fd)

    # Redirecting stdout to the destination file allows use of print() and
    # automatic addition of '\n' to each line.  Also allows for debugging
    # by printing to stdout instead.
    if outputCsvFile is not None:
        try:
            fd = open(outputCsvFile, 'w')
        except IOError:
            print(TextColors.FAIL + 'Unable to create file %(outFile)s' % 
                  {'outFile': outputJavaFile})
            return False

        sys.stdout = fd

    # Generate Column Headers
    print('Error Code,Internal Software Tag,Explanation,Possible Root Cause')

    # Generate Error Codes
    for errorSource in jsonObj['ErrorSources']:
        if not 'errors' in errorSource:
            continue

        for errorDef in errorSource['errors']:
            isError = errorDef.get('isError', True)
            if isError:
                value = THOR_ERROR | ( int(errorSource['value'], 16) << 16) | int(errorDef['value'], 16)

                cause = errorDef.get('cause', '')

                print('0x%(value)X,%(label)s,%(description)s,%(cause)s' %
                      {'label': errorDef['label'],
                       'value': value,
                       'description': errorDef['description'],
                       'cause': cause})

        print(',,,,')

    if outputCsvFile is not None:
        sys.stdout = sys.__stdout__
        print('CSV file generated: %(outFile)s' % 
              {'outFile': outputCsvFile} )

    return True

#------------------------------------------------------------------------------
def main():
    #if 2 != len(sys.argv):
    #    usage()
    #    return
    #
    #inputFile = sys.argv[1]

    if not validateJson(JSON_FILE):
        return

    if not genHeaderFile(JSON_FILE, C_HEADER_FILE):
        return

    # This line is for testing with stdout
    #genHeaderFile(JSON_FILE)

    if not genJavaFile(JSON_FILE, JAVA_CLASS_FILE):
        return

    # This line is for testing with stdout
    #genJavaFile(JSON_FILE)

    if not genCsvFile(JSON_FILE, CSV_FILE):
        return

    # This line is for testing with stdout
    #genCsvFile(JSON_FILE)

    print(TextColors.SUCCESS + 'Succeeded')

#------------------------------------------------------------------------------
main()
