package com.echonous.ai;

public class BVCaliper {
    BVPoint start;
    BVPoint end;
    public BVCaliper(BVPoint s, BVPoint e){
        start = s;
        end = e;
    }

    public BVPoint getStart() {
        return start;
    }

    public BVPoint getEnd() {
        return end;
    }
}
