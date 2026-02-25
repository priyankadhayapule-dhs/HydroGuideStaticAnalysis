# Python script to test sync of US, Ecg and DA
#
# This script does the following
#
# 1. check adb is running with root privilage
# 2. check get thor files from Thor device
# 3. get list of Thor files
#
#
#

import os
from os import listdir
from os.path import isfile, join
import thorreader
import numpy as np


def daecgSyncTest(in_file):
    tr = thorreader.ThorReader(in_file)
    tr.setDisplayFileInfo(False)
    tr.readFile()
    tsB = tr.readBTimeStamp()
    tsEcg = tr.readECGTimeStamp()
    tsDa = tr.readDATimeStamp()
    data_type = tr.getDataType();
    #
    da_data_type = (data_type >> 7 & 1)
    ecg_data_type =  (data_type >> 8 & 1)
    #
    # constant: tick frequency and timespan
    tick_freq = 2500*pow(10,6)/24.0
    tick_time = 1.0/tick_freq

    #test_pass
    test_pass = True;

    # check frameInterval of B
    tsB_diff = tsB[1:] - tsB[:-1]
    B_frameInterval_ms = np.mean(tsB_diff) * tick_time * 1000

    if (np.floor(B_frameInterval_ms) != tr.getFrameInterval()):
        print("B frameInterval mismatch")
        test_pass = False

    if (np.std(tsB_diff) != 0.0):
        print("B frameInterval Variability detected")
        test_pass = False

    # Ecg
    if (ecg_data_type):
        tsBEcg_diff = tsB - tsEcg
        print("B and Ecg time difference (ms): {0:4.4f}".format(np.mean(tsBEcg_diff) * tick_time * 1000))
        #
        if (np.mean(tsB_diff)/2.0 < np.mean(tsBEcg_diff)):
            print("B and Ecg out of sync detected")
            test_pass = False
        #
        if (np.std(tsBEcg_diff) != 0.0):
            print("B and Ecg sync Variability detected")
            test_pass = False

    if (da_data_type):
        tsBDa_diff = tsB - tsDa
        print("B and Da time difference (ms): {0:4.4f}".format(np.mean(tsBDa_diff) * tick_time * 1000))
        #
        if (np.mean(tsB_diff)/2.0 < np.mean(tsBDa_diff)):
            print("B and Da out of sync detected")
            test_pass = False
        # variability
        if (np.std(tsBDa_diff) != 0.0):
            print("B and Da sync Variability detected")
            test_pass = False

    return test_pass


# check whether adb is running
result = os.system("adb root")

if (result == 0):
    print("verified that adb is runnig.")
else:
    print("please verify whether the device is running.")
    raise Exception("device is not connected.")

#
print("----- copying Thor files -----")
# copying thor files
result = os.system("adb pull /data/data/com.echonous.kosmosapp/clips .")

if (result == 0):
    print("copying Thor clips is successful.")
else:
    raise Exception("copying Thor files from the device failed.")

#
clippath = 'clips'
thorfiles = [f for f in listdir(clippath) if ((isfile(join(clippath, f))) & (os.path.splitext(f)[1] == '.thor')) ]
print("found", len(thorfiles), "Thor files")

test_file_cnt = 0;
pass_file_cnt = 0;

for thorfile in thorfiles:
    test_file = join(clippath, thorfile)
    tr = thorreader.ThorReader(test_file)
    tr.setDisplayFileInfo(False)
    tr.readFile()

    # test Thor files with Da and Ecg
    if ((tr.getDataType() == 385) | (tr.getDataType() == 129) | (tr.getDataType() == 257)):
        print("Processing: ", test_file)
        test_file_cnt += 1

        if (daecgSyncTest(test_file)):
            print("Test Passed")
            pass_file_cnt += 1
        else:
            print("Test Failed")

print("Number of files tested: ", test_file_cnt, "passed: ", pass_file_cnt)
