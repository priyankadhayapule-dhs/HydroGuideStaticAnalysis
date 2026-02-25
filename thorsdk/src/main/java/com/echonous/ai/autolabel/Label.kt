package com.echonous.ai.autolabel

import kotlinx.serialization.Serializable

@Serializable
data class Label(val id: Int, val x: Float, val y: Float, val w: Float, val h: Float, val conf: Float)
{
    val isValid = id >= 0
    companion object {
        val invalid = Label(-1, 0f, 0f, 0f, 0f, 0f)
    }
}