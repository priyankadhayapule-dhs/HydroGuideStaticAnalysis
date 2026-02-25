# Scan converter
import matplotlib.pyplot as plt
import numpy as np
import scanconverter
import thorreader

f_name = "06182019cf1c299e-27ba-412a-a9d4-4d01984410a3.thor"

# read file - SC Params from dB then ReadBFrames
# !!!!! ORDER MATTERS !!!!!!
tr = thorreader.ThorReader(f_name)
tr.readFile()
sc_params = tr.readBSCParams();
b_data = tr.readThorBFrames()
#
imagingDepth = sc_params['startSampleMm'] + (sc_params['numSamples'] - 1) * sc_params['sampleSpacingMm']
sc = scanconverter.ScanConverter()
sc.setParams(imagingDepth, sc_params['numSamples'], sc_params['numRays'], sc_params['startSampleMm'], sc_params['sampleSpacingMm'], sc_params['startRayRadian'] * 180/np.pi, sc_params['raySpacingRadian'] * 180/np.pi, 256, 256, 0, 0)
sc_out = sc.convert((b_data[0,:,:]))
plt.imshow(sc_out)
plt.show()


imp.reload(thorreader)

imp.reload(scanconverter)
sc = scanconverter.ScanConverter()
sc.setParams(imagingDepth, sc_params['numSamples'], sc_params['numRays'], sc_params['startSampleMm'], sc_params['sampleSpacingMm'], sc_params['startRayRadian'] * 180/np.pi, sc_params['raySpacingRadian'] * 180/np.pi, 256, 256, 0, 0)
sc_out = sc.convert((b_data[0,:,:]))
