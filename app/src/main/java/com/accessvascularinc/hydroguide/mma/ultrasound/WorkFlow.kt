package com.accessvascularinc.hydroguide.mma.ultrasound

class WorkFlow{
    var id: Int = -1
    var globalId: Int = -1
    lateinit var job: String
    lateinit var method: String
    var organ: Int = -1
    var view: Int = -1
    var mode: Int = -1

    var defaultDepthEasy: Int = -1
    var defaultDepthGeneral: Int = -1
    var defaultDepthHard: Int = -1

    lateinit var organName: String


}