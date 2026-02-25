#!python
#!/usr/bin/env python
#

from scipy.io import loadmat
import os
from os import listdir
from os.path import isfile, join, isdir
import numpy as np
import heartpy as hp
from scipy import signal

input_dir = "MLII"
processdir(input_dir)



def processfile(in_file):
    # process file
    try:
        #print(in_file)
        in_data = loadmat(in_file)['val']
        data_size = in_data.size
        r_data = in_data.reshape(data_size)
        #
        working_data, measures = hp.process(r_data, 360.0)

        roll_mean = working_data['rolling_mean']

        #if ((working_data['removed_beats'].size +  working_data['removed_beats_y'].size) > 0):
        #if (((measures['rmssd']/measures['bpm']) > 0.55) | ((working_data['removed_beats'].size +  working_data['removed_beats_y'].size) > 2) | (np.std(roll_mean)/np.mean(roll_mean) > 0.04 ))  :

        peak_mask = working_data['binary_peaklist']
        peaks = working_data['ybeat']

        real_peak = []
        for i in range(len(peak_mask)):
            if (peak_mask[i] == 1):
                real_peak.append(peaks[i])

        std_ratio = np.std(real_peak)/(np.mean(real_peak) - np.mean(roll_mean))

        if ((std_ratio > 0.09) | (working_data['removed_beats'].size > 1) | ((measures['rmssd']/measures['bpm']) > 0.50) | (measures['sdnn'] > 36.0) ) :
            return

        #
        bpm = measures['bpm']
        f_name = in_file.replace("/", "_")
        f_name = f_name.replace(" ", "_")
        f_name = f_name.replace(".", "_")
        f_name = f_name + "_"
        bpm_str = "{:06.2f}".format(bpm)
        #
        f_name = f_name + bpm_str.replace(".", "_")
        f_name += ".dat"
        #print(f_name)
        #
        #normlalize data
        rn_data = (r_data - np.mean(r_data)) / (max(r_data) - np.mean(r_data)) * 0.5
        #
        target_sampling = int(np.floor(data_size*397.36/360.0))
        rsn_data = signal.resample(rn_data, target_sampling)
        #
        out_dir = "MLII_out"
        #
        np.savetxt(join(out_dir, f_name), rsn_data, fmt="%5.6f")
    except RuntimeError:
        print("Error Occurred in processing ", in_file)




def processdir(in_dir):
    cont = listdir(in_dir)

    for f in cont:
        comb_path = join(in_dir, f)

        if(isdir(comb_path)):
            processdir(comb_path)
        elif(isfile(comb_path)):
            # process file
            if (os.path.splitext(comb_path)[-1] == ".mat"):
                processfile(comb_path)
            #print(comb_path)
