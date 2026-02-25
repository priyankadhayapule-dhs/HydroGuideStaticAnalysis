package com.echonous.ai.guidance

import com.echonous.ai.MLModel
import kotlinx.serialization.Serializable

@Serializable
data class Config(
    val modelName: String,
    val modelDescription: MLModel.Description,
    val imageWidth: Int,
    val imageHeight: Int,
) {

    companion object {
        val a4c_finetuned = Config(
            modelName = "guidance-a4c.tflite",
            modelDescription = MLModel.Description(
                name = "Grading-Guidance-A4C-finetuned",
                version = "2024-04-11",
                inputs = listOf(MLModel.Feature(name = "image", shape = intArrayOf(1, 1, 128, 128))),
                outputs = listOf(
                    MLModel.Feature(name = "subview", shape = intArrayOf(1, 23)),
                    MLModel.Feature(name = "quality", shape = intArrayOf(1, 5)),
                ),
            ),
            imageWidth = 128,
            imageHeight = 128,
        )
        val a2c_finetuned = Config(
            modelName = "guidance-a2c.tflite",
            modelDescription = MLModel.Description(
                name = "Grading-Guidance-A2C-finetuned",
                version = "2024-04-11",
                inputs = listOf(MLModel.Feature(name = "image", shape = intArrayOf(1, 1, 128, 128))),
                outputs = listOf(
                    MLModel.Feature(name = "subview", shape = intArrayOf(1, 23)),
                    MLModel.Feature(name = "quality", shape = intArrayOf(1, 5)),
                ),
            ),
            imageWidth = 128,
            imageHeight = 128,
        )
        val plax = Config(
            modelName = "plax_v10.tflite",
            modelDescription = MLModel.Description(
                name = "2022_06_10_PLAX_SingleBranch",
                version = "1.0",
                inputs = listOf(MLModel.Feature(name = "image", shape = intArrayOf(1, 1, 224, 224))),
                outputs = listOf(
                    MLModel.Feature(name = "subview", shape = intArrayOf(1, 21)),
                    MLModel.Feature(name = "quality", shape = intArrayOf(1, 5)),
                ),
            ),
            imageWidth = 224,
            imageHeight = 224,
        )
    }

    override fun equals(other: Any?): Boolean {
        // Auto generated equals method required because of FloatArray property
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as Config

        if (modelName != other.modelName) return false
        if (imageWidth != other.imageWidth) return false
        if (imageHeight != other.imageHeight) return false

        return true
    }

    override fun hashCode(): Int {
        // Auto generated hashCode method required because of FloatArray property
        var result = modelName.hashCode()
        result = 31 * result + imageWidth
        result = 31 * result + imageHeight
        return result
    }
}