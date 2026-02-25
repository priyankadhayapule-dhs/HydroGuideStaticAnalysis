package com.accessvascularinc.hydroguide.mma.ultrasound

import android.graphics.drawable.Drawable

data class ColorMapModel(var colorMap: Drawable?, var colorMapInverted: Drawable?, var bytes: ByteArray, var isVelocity: Boolean, var isSelected: Boolean, var index: Int) {

    var isPow = false

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as ColorMapModel

        if (colorMap != other.colorMap) return false
        if (!bytes.contentEquals(other.bytes)) return false
        if (isVelocity != other.isVelocity) return false
        if (isSelected != other.isSelected) return false
        if (index != other.index) return false

        return true
    }

    override fun hashCode(): Int {
        var result = colorMap.hashCode()
        result = 31 * result + bytes.contentHashCode()
        result = 31 * result + isVelocity.hashCode()
        result = 31 * result + isSelected.hashCode()
        result = 31 * result + index
        return result
    }
}