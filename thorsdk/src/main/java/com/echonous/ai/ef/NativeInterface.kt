package com.echonous.ai.ef

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.Tensor
import com.echonous.ai.autolabel.Pipeline
import java.nio.ByteBuffer
import com.echonous.ai.ef.PhaseDetectPipeline
import com.echonous.ai.tflite.TfLiteProvider
import java.nio.ByteOrder

class NativeInterface(val context: Context): AutoCloseable {
    var phaseDetectPipeline: PhaseDetectPipeline? = null
    var edSegmentationPipeline: LVSegmentationPipeline? = null
    var esSegmentationPipeline: LVSegmentationPipeline? = null

    private data class EFPipelines(
        val phaseDetect: PhaseDetectPipeline,
        val edSegmentation: LVSegmentationPipeline,
        val esSegmentation: LVSegmentationPipeline
    )

    fun load() {
        val pipelines = createTfLiteModels(context)
        phaseDetectPipeline = pipelines.phaseDetect
        edSegmentationPipeline = pipelines.edSegmentation
        esSegmentationPipeline = pipelines.esSegmentation
    }

    /// Run phase detection model on a scan converted image
    fun runPhaseDetection(image: ByteBuffer): Float {
        val pipeline = phaseDetectPipeline ?: return 0.0f
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val modelPred = pipeline.process(imageTensor)
        return modelPred
    }

    /// Run segmentation ensemble for ES frames
    /// on a scan converted buffer of 5 frames
    fun runSegmentationES(image: ByteBuffer, confMap: ByteBuffer) {
        val pipeline = esSegmentationPipeline ?: return
        runSegmentationGeneric(pipeline, image, confMap)
    }

    /// Run segmentation ensemble for ES frames
    /// on a scan converted buffer of 5 frames
    fun runSegmentationED(image: ByteBuffer, confMap: ByteBuffer) {
        val pipeline = edSegmentationPipeline ?: return
        runSegmentationGeneric(pipeline, image, confMap)
    }

    private fun runSegmentationGeneric(pipeline: LVSegmentationPipeline, image: ByteBuffer, confMap: ByteBuffer) {
        val inputShape = pipeline.models[0].description.inputs[0].shape
        val outputShape = pipeline.models[0].description.outputs[0].shape.clone()
        outputShape[0] = pipeline.models.size
        image.order(ByteOrder.nativeOrder())
        confMap.order(ByteOrder.nativeOrder())
        pipeline.process(
            Tensor(inputShape, image.asFloatBuffer()),
            Tensor(outputShape, confMap.asFloatBuffer())
        )
    }

    override fun close() {
        // destroy models/other things here
        esSegmentationPipeline?.close()
        edSegmentationPipeline?.close()
        phaseDetectPipeline?.close()
        esSegmentationPipeline = null
        edSegmentationPipeline = null
        phaseDetectPipeline = null
    }

    companion object {


        private fun createTfLiteModels(context: Context) : EFPipelines {
            // Do TFLite initialization here...
            val provider = TfLiteProvider.createGMSProvider(context)
            val phaseDetect = PhaseDetectPipeline.phaseDetectPipelineDefault(provider, context)
            val edSegmentation = LVSegmentationPipeline.edPipeline(provider, context)
            val esSegmentation = LVSegmentationPipeline.esPipeline(provider, context)
            return EFPipelines(phaseDetect, edSegmentation, esSegmentation)
        }
    }

}