package com.echonous.ai.autopreset
import android.content.Context

import com.echonous.ai.Tensor
import com.echonous.ai.tflite.TfLiteProvider
import java.nio.ByteBuffer
import java.nio.ByteOrder

class NativeInterface(val context: Context): AutoCloseable {
    var pipeline: Pipeline? = null

    fun process(image: ByteBuffer, labels: ByteBuffer) : Int{
        val startTime = System.currentTimeMillis()
        val pipeline = pipeline ?: return -1
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val modelPreds = pipeline.process(imageTensor)
        labels.order(ByteOrder.nativeOrder())
        labels.clear()
        for (label in modelPreds) {
            labels.putFloat(label.toFloat())
        }
        labels.flip()
        val elapsedTime = System.currentTimeMillis() - startTime
        return 0
    }
    fun load() {
        pipeline = createTfLitePipeline(context)
    }
    override fun close() {
        pipeline?.close()
    }
    companion object {
        fun createTfLitePipeline(context: Context) : Pipeline{
            val  provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipeline20240205(provider, context)
        }
    }
}