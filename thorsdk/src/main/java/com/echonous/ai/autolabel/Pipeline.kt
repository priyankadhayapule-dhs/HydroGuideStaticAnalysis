package com.echonous.ai.autolabel

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor

data class PipelineResult(
    val view: Tensor,
    val labels: List<Label>,
    val smoothView: Tensor,
    val smoothLabels: List<Label>,
    val yoloRaw: List<SimplePrediction> ?= null,
    val yoloProc: List<SimplePrediction> ?= null
)

class Pipeline(val model: MLModel, val config: Config): AutoCloseable {
    private val smoothingBuffer = SmoothingBuffer(config.smoothingFrames, config)

    fun clear() {
        smoothingBuffer.clear()
    }

    fun process(image: Tensor): List<Label> {
        return processWithIntermediates(image).smoothLabels
    }

    fun processWithIntermediates(image: Tensor): PipelineResult {
        model.predict(listOf(image)).use {
            val yolo = it[0]
            val view = it[1]
            val labels = processYolo(yolo, config)

            processView(view) // in place
            smoothingBuffer.add(view, labels)
            return PipelineResult(view, labels, smoothingBuffer.smoothView, smoothingBuffer.labels, null, null)
        }
    }

    override fun close() {
        model.close()
    }

    companion object {
        fun pipelineV36(provider: MLProvider, context: Context): Pipeline {
            return pipelineFromConfig(Config.version_3_6, provider, context)
        }

        fun pipelineV40(provider: MLProvider, context: Context): Pipeline {
            return pipelineFromConfig(Config.version_4_0, provider, context)
        }

        fun pipeline_tdi(provider: MLProvider, context: Context): Pipeline {
            return pipelineFromConfig(Config.version_tdi, provider, context)
        }

        fun pipelineFromConfig(config: Config, provider: MLProvider, context: Context): Pipeline {
            val model = provider.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
            return Pipeline(model, config)
        }
    }
}