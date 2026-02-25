package com.accessvascularinc.hydroguide.mma.ultrasound

class Release(releaseType: Int, releaseDate: String) {

    var releaseType: Int? = 0
    var releaseDate: String? = null

    init {
        this.releaseType = releaseType
        this.releaseDate = releaseDate
    }
}