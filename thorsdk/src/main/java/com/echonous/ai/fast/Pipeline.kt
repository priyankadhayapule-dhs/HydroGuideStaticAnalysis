package com.echonous.ai.fast

import android.content.Context

import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor

class Pipeline(val model: MLModel, val config: com.echonous.ai.fast.Config): AutoCloseable {
   // private val smoothingBuffer = SmoothingBuffer(config.smoothingFramesCount, config)

    fun clear() {
       // smoothingBuffer.clear()
    }

    fun process(image: Tensor): FASTPrediction {
        val inputRange = image.getRangeForDebug()
        model.predict(listOf(image)).use {
            val view = it[1]
            val yolo = it[0]
//            Log.d("FAST Pipeline", "YoloShape: ${yolo.shape[0]}, ${yolo.shape[1]}, ${yolo.shape[2]}, ${yolo.shape[3]}")
//            Log.d("FAST Pipeline", "${config.modelDescription.name} View Length: ${view.size} View Shape: ${view.shape.size} Yolo Length: ${yolo.size}")
            return FASTPrediction((view.data.array().asList()), (yolo.data.array().asList()))
        }
    }

    fun processYolo(yolo: List<Float>) {

    }

    fun processDebug(image: Tensor) : FASTPrediction {
        val yoloLength = 128 * 128 * 3 * 23
        val viewLength = 15
        val yolo = ArrayList<Float>(yoloLength)
        val view = ArrayList<Float>(viewLength)
        return FASTPrediction(yolo, view)
    }

    fun parseYolo(modelOut : MLModel.Output) {

    }

    override fun close() {
        model.close()
    }

    companion object {

        fun pipelineFromConfig(config: com.echonous.ai.fast.Config, provider: MLProvider, context: Context): Pipeline {
            val model = provider.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
            return Pipeline(model, config)
        }

        fun default(provider: MLProvider, context: Context): Pipeline {
            val model = provider.createModelFromAsset(context.assets, com.echonous.ai.fast.Config.fast_142704.modelName, com.echonous.ai.fast.Config.fast_142704.modelDescription)
            return Pipeline(model, com.echonous.ai.fast.Config.fast_142704)
        }

    }
}