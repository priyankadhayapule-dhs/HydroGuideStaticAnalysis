package com.echonous.ai.bladder
import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.Tensor
import com.echonous.ai.bladder.Pipeline
import com.echonous.ai.tflite.TfLiteProvider
import com.echonous.util.LogUtils
import java.nio.ByteBuffer
import java.nio.ByteOrder

class NativeInterface(val context: Context) : AutoCloseable {
    var hybridModel: MutableList<Pipeline?> = mutableListOf()

    private var currentIndex: Int = 0
    fun load() {
        //pipeline =
        hybridModel.add(createTfLitePipeline(context, "bvw_1_nobuco.tflite"))
        hybridModel.add(createTfLitePipeline(context, "bvw_2_nobuco.tflite"))
        hybridModel.add(createTfLitePipeline(context, "bvw_3_nobuco.tflite"))
        hybridModel.add(createTfLitePipeline(context, "bvw_4_nobuco.tflite"))
    }

    fun processOrdered(image: ByteBuffer, labels: ByteBuffer, pMap: ByteBuffer, index: Int) {
        val pipeline = hybridModel[index] ?: return
        image.order(ByteOrder.nativeOrder())
        labels.order(ByteOrder.nativeOrder())
        pMap.order(ByteOrder.nativeOrder())
        pMap.clear()
        labels.clear()
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val result = pipeline.process(imageTensor)

        writeTensorToByteBuffer(result.pMapOut, pMap)
        writeTensorToByteBuffer(result.viewOut, labels)
        pMap.flip()
        labels.flip()
        return
    }

    fun process(image: ByteBuffer, views: ByteBuffer, pMap: ByteBuffer){

        val ret = processOrdered(image, views, pMap, currentIndex)
        currentIndex += 1
        if(currentIndex >= hybridModel.size)
            currentIndex = 0
        return
    }

    fun writeTensorToByteBuffer(tensor: Tensor, byteBuffer: ByteBuffer) {
        require(tensor.size <= byteBuffer.remaining() * Float.SIZE_BYTES)
        for (i in 0 until tensor.size) {
            byteBuffer.putFloat(tensor.data.get(i))
        }
    }

    override fun close() {
        return
    }

    companion object {
        fun createTfLitePipeline(context: Context, modelName: String): com.echonous.ai.bladder.Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            val modelInput = MLModel.Feature("image", intArrayOf(1,1,128,128))
            val modelViewOutput = MLModel.Feature("view", intArrayOf(1,5))
            val modelPmapOutput = MLModel.Feature("pMap", intArrayOf(1,2,128,128))
            val modelOutput = MLModel.Feature("debug", intArrayOf(1,64,16,16))
            val inputsList = listOf<MLModel.Feature>(modelInput)
            val outputsList = listOf<MLModel.Feature>(modelPmapOutput, modelViewOutput, modelOutput)
            val modelDescription = MLModel.Description(modelName,  "1.0.0", inputsList, outputsList)
            val config = Config(modelName, modelDescription, 128, 128, emptyList<String>(),5)
            return com.echonous.ai.bladder.Pipeline.pipelineFromConfig(config, provider, context)
        }
    }
}