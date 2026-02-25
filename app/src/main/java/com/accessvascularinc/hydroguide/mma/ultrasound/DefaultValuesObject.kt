package com.accessvascularinc.hydroguide.mma.ultrasound;

class DefaultValuesObject {
    private var defaultDepth : Int = -1
    private var defaultDepthId : Int = -1
    private var defaultView : Int = -1

    constructor(defaultDepthId: Int, defaultView: Int) {
        this.defaultDepthId = defaultDepthId
        this.defaultView = defaultView
    }

    fun getDefaultDepthId(): Int {
        return defaultDepthId
    }

    fun getDefaultView(): Int {
        return defaultView
    }
}
