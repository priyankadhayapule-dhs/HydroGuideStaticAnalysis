package com.echonous.ai.bladder

import com.echonous.ai.MLModel
import kotlinx.serialization.Serializable

@Serializable
data class Config(
    val modelName: String,
    val modelDescription: MLModel.Description,
    val imageWidth: Int,
    val imageHeight: Int,
    val viewNames: List<String>,
    val viewBufferSize: Int
) {
}