package com.echonous.ai.autolabel

import android.content.Context
import com.echonous.ai.Tensor
import com.echonous.ai.tflite.TfLiteProvider
import java.nio.ByteBuffer
import java.nio.ByteOrder

// Interface designed to be easy to use from native side code
// Does not throw exceptions, simple data types as arguments when possible, etc
class NativeInterface(val context: Context): AutoCloseable {
    var pipeline: Pipeline? = null
    var pipelinePW: Pipeline? = null
    var pipelineTDI: Pipeline? = null

    fun load() {
        pipeline = createTfLitePipeline(context)
        pipelinePW = createTfLitePipelinePW(context)
        pipelineTDI = createTfLitePipelineTDI(context)
    }

    // Process an image, writing the labels into the labels buffer.
    // Returns the number of labels written
    fun process(image: ByteBuffer, labels: ByteBuffer): Int {
        val pipeline = pipeline ?: return 0
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val labelsList = pipeline.process(imageTensor)
        labels.order(ByteOrder.nativeOrder())
        labels.clear()
        for (label in labelsList) {
            labels.putInt(label.id)
            labels.putFloat(label.x)
            labels.putFloat(label.y)
            labels.putFloat(label.w)
            labels.putFloat(label.h)
            labels.putFloat(label.conf)
        }
        labels.flip()
        return labelsList.size
    }
    // Process an image, writing the labels into the labels buffer.
    // Returns the number of labels written
    fun processPW(image: ByteBuffer, labels: ByteBuffer): Int {
        val pipelinePW = pipelinePW ?: return 0
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipelinePW.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val labelsList = pipelinePW.process(imageTensor)
        labels.order(ByteOrder.nativeOrder())
        labels.clear()
        for (label in labelsList) {
            labels.putInt(label.id)
            labels.putFloat(label.x)
            labels.putFloat(label.y)
            labels.putFloat(label.w)
            labels.putFloat(label.h)
            labels.putFloat(label.conf)
        }
        labels.flip()

        return labelsList.size
    }

    // Process an image, writing the labels into the labels buffer.
    // Returns the number of labels written
    fun processTDI(image: ByteBuffer, labels: ByteBuffer): Int {
        val pipelineTDI = pipelineTDI ?: return 0
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipelineTDI.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val labelsList = pipelineTDI.process(imageTensor)
        labels.order(ByteOrder.nativeOrder())
        labels.clear()
        for (label in labelsList) {
            labels.putInt(label.id)
            labels.putFloat(label.x)
            labels.putFloat(label.y)
            labels.putFloat(label.w)
            labels.putFloat(label.h)
            labels.putFloat(label.conf)
        }
        labels.flip()

        return labelsList.size
    }

    // Process an image writing the results including unsmoothed intermediates into a bytebuffer
    // This bytebuffer MUST match the expected format (see CardiacAnnotatorTask.cpp struct PipelineFullResults):
    //   buffer for unsmooth views (array of floats, config.views.size long)
    //   int for number of unsmooth labels
    //   buffer for unsmooth labels (array of Label, config.labels.size long)
    //   buffer for smooth views (array of floats, config.views.size long)
    //   int for number of smooth labels
    //   buffer for smooth labels (array of Label, config.labels.size long)
    fun processTDIWithIntermediates(image: ByteBuffer, dataBuffer: ByteBuffer) {
        val pipeline = pipelineTDI ?: return
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val result = pipeline.processWithIntermediates(imageTensor)
        dataBuffer.order(ByteOrder.nativeOrder())
        dataBuffer.clear()
        // Write all data into buffer. The lists of Labels are fixed length (max config.labels.size)
        writeTensorToByteBuffer(result.view, dataBuffer)
        writeLabelsToByteBufferFixedSize(result.labels, pipeline.config.views.size, dataBuffer)

        writeTensorToByteBuffer(result.smoothView, dataBuffer)
        writeLabelsToByteBufferFixedSize(result.smoothLabels, pipeline.config.views.size, dataBuffer)
        writePredictionsToByteBufferFixedSize(result.yoloRaw, dataBuffer, 20, pipeline.config)
        writePredictionsToByteBufferFixedSize(result.yoloProc, dataBuffer, 20, pipeline.config)
        dataBuffer.flip()
    }
    fun processPWWithIntermediates(image: ByteBuffer, dataBuffer: ByteBuffer) {
        val pipeline = pipelinePW ?: return
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val result = pipeline.processWithIntermediates(imageTensor)
        dataBuffer.order(ByteOrder.nativeOrder())
        dataBuffer.clear()

        // Write all data into buffer. The lists of Labels are fixed length (max config.labels.size)
        writeTensorToByteBuffer(result.view, dataBuffer)
        writeLabelsToByteBufferFixedSize(result.labels, pipeline.config.labels.size, dataBuffer)

        writeTensorToByteBuffer(result.smoothView, dataBuffer)
        writeLabelsToByteBufferFixedSize(result.smoothLabels, pipeline.config.labels.size, dataBuffer)
        writePredictionsToByteBufferFixedSize(result.yoloRaw, dataBuffer, 20, pipeline.config)
        writePredictionsToByteBufferFixedSize(result.yoloProc, dataBuffer, 20, pipeline.config)
        dataBuffer.flip()
    }
    // Process an image writing the results including unsmoothed intermediates into a bytebuffer
    // This bytebuffer MUST match the expected format (see CardiacAnnotatorTask.cpp struct PipelineFullResults):
    //   buffer for unsmooth views (array of floats, config.views.size long)
    //   int for number of unsmooth labels
    //   buffer for unsmooth labels (array of Label, config.labels.size long)
    //   buffer for smooth views (array of floats, config.views.size long)
    //   int for number of smooth labels
    //   buffer for smooth labels (array of Label, config.labels.size long)
    fun processWithIntermediates(image: ByteBuffer, dataBuffer: ByteBuffer) {
        val pipeline = pipeline ?: return
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val result = pipeline.processWithIntermediates(imageTensor)
        dataBuffer.order(ByteOrder.nativeOrder())
        dataBuffer.clear()

        // Write all data into buffer. The lists of Labels are fixed length (max config.labels.size)
        writeTensorToByteBuffer(result.view, dataBuffer)
        writeLabelsToByteBufferFixedSize(result.labels, pipeline.config.labels.size, dataBuffer)

        writeTensorToByteBuffer(result.smoothView, dataBuffer)
        writeLabelsToByteBufferFixedSize(result.smoothLabels, pipeline.config.labels.size, dataBuffer)

        writePredictionsToByteBufferFixedSize(result.yoloRaw, dataBuffer, 20, pipeline.config)
        writePredictionsToByteBufferFixedSize(result.yoloProc, dataBuffer, 20, pipeline.config)

        dataBuffer.flip()
    }
    private fun writePredictionsToByteBufferFixedSize( preds: List<SimplePrediction>?, byteBuffer: ByteBuffer, fixedSize: Int, config: Config){
        if (preds == null)
            return
        val bytesPerPred = 6 * Float.SIZE_BYTES
        require(Int.SIZE_BYTES + fixedSize*bytesPerPred <= byteBuffer.remaining())
        // First, write number of valid labels

        // Then write all labels
        // always write fixed size so we always take the same amount of space in the buffer
        // Once past end of list, we write invalid labels
        val invalidLblConf = -1.0f
        for(i in 0 until preds.size) {
            val pred = preds[i]
            byteBuffer.putFloat(pred.x)
            byteBuffer.putFloat(pred.y)
            byteBuffer.putFloat(pred.w)
            byteBuffer.putFloat(pred.h)
            byteBuffer.putFloat(pred.objConf)
            for(lbl in 0 until fixedSize){
                val conf = if (lbl < config.labels.size) { pred.labelConf[lbl] } else { invalidLblConf }
                byteBuffer.putFloat(conf)
            }
        }
    }
    private fun writeTensorToByteBuffer(tensor: Tensor, byteBuffer: ByteBuffer) {
        require(tensor.size <= byteBuffer.remaining() * Float.SIZE_BYTES)
        for (i in 0 until tensor.size) {
            byteBuffer.putFloat(tensor.data.get(i))
        }
    }

    private fun writeLabelsToByteBufferFixedSize(labels: List<Label>, fixedSize: Int, byteBuffer: ByteBuffer) {
        val bytesPerLabel = Int.SIZE_BYTES + 5 * Float.SIZE_BYTES
        require(Int.SIZE_BYTES + fixedSize*bytesPerLabel <= byteBuffer.remaining())
        // First, write number of valid labels
        byteBuffer.putInt(labels.size)

        // Then write all labels
        // always write fixed size so we always take the same amount of space in the buffer
        // Once past end of list, we write invalid labels
        for (i in 0 until fixedSize) {
            val label = if (i < labels.size) { labels[i] } else { Label.invalid }
            byteBuffer.putInt(label.id)
            byteBuffer.putFloat(label.x)
            byteBuffer.putFloat(label.y)
            byteBuffer.putFloat(label.w)
            byteBuffer.putFloat(label.h)
            byteBuffer.putFloat(label.conf)
        }
    }

    fun clear() {
        pipeline?.clear()
        pipelineTDI?.clear()
        pipelinePW?.clear()
    }

    override fun close() {
        pipeline?.close()
    }

    companion object {
        fun createTfLitePipeline(context: Context): Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipelineV36(provider, context)
        }
        fun createTfLitePipelineTDI(context: Context): Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipeline_tdi(provider, context)
        }
        fun createTfLitePipelinePW(context: Context): Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipelineV40(provider, context)
        }
    }
}