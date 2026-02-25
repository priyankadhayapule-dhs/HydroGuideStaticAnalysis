package com.echonous.ai;

import java.util.Arrays;

public final class BVSegmentation {
    BVCaliper majorCaliper; // width traverse or length sagittal
    BVCaliper minorCaliper; // depth
    float[] contour; // [x1, y1, x2, y2, ... ]
    public BVSegmentation(float[] c, BVCaliper majC, BVCaliper minC)
    {
        majorCaliper = majC;
        minorCaliper = minC;
        contour = c;
    }
    public String contourInfo()
    {
        return Arrays.toString(contour);
    }

    public BVCaliper getMajorCaliper() {
        return majorCaliper;
    }

    public BVCaliper getMinorCaliper() {
        return minorCaliper;
    }

    public float[] getContour() {
        return contour;
    }
}
