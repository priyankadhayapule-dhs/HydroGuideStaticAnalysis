package com.echonous.ai.guidance

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.Tensor
import com.echonous.ai.tflite.TfLiteProvider
import java.nio.ByteBuffer
import java.nio.ByteOrder

class NativeInterface(val context: Context) {
    var modelA4C: MLModel? = null
    var modelA2C: MLModel? = null
    var modelPLAX: MLModel? = null

    val configA4C = Config.a4c_finetuned
    val configA2C = Config.a2c_finetuned
    val configPLAX = Config.plax

    fun load() {
        modelA4C = createTfLiteGGModel(context, configA4C)
        modelA2C = createTfLiteGGModel(context, configA2C)
        modelPLAX = createTfLiteGGModel(context, configPLAX)
    }

    /// Process/predict a singple frame with A4C model
    /// To reduce number of buffers, outputs are concatenated together (subview, quality)
    fun processA4C(image: ByteBuffer, outputs: ByteBuffer) {
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(configA4C.modelDescription.inputs[0].shape, image.asFloatBuffer())
        process(imageTensor, outputs, modelA4C!!)
    }

    /// Process/predict a singple frame with A2C model
    /// To reduce number of buffers, outputs are concatenated together (subview, quality)
    fun processA2C(image: ByteBuffer, outputs: ByteBuffer) {
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(configA2C.modelDescription.inputs[0].shape, image.asFloatBuffer())
        process(imageTensor, outputs, modelA2C!!)
    }

    fun processPLAX(image: ByteBuffer, outputs: ByteBuffer) {
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(configPLAX.modelDescription.inputs[0].shape, image.asFloatBuffer())
        process(imageTensor, outputs, modelPLAX!!)
    }

    fun process(image: Tensor, outputs: ByteBuffer, model: MLModel) {
        outputs.order(ByteOrder.nativeOrder())
        val outputsFloats = outputs.asFloatBuffer()
        outputsFloats.clear();
        model.predict(listOf(image)).use {
            val subviewTensor = it[0]
            val qualityTensor = it[1]

            outputsFloats.put(subviewTensor.data)
            outputsFloats.put(qualityTensor.data)
        }
    }

    companion object {
        @JvmStatic
        fun createTfLiteGGModel(context: Context, config: Config): MLModel {
            val provider = TfLiteProvider.createGMSProvider(context)
            return provider.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
        }
    }
}