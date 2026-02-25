package com.echonous.ai.vti

import android.content.Context
import com.echonous.ai.Tensor
import java.nio.ByteBuffer
import java.nio.ByteOrder
import com.echonous.ai.tflite.TfLiteProvider
import com.echonous.util.LogUtils
import java.nio.FloatBuffer

class NativeInterface (val context: Context) : AutoCloseable{

    var pipeline_38_38_88_170: Pipeline? = null
    var pipeline_36_51_50_167: Pipeline? = null
    var pipeline_50_33_50_154: Pipeline? = null
    override fun close() {
        pipeline_38_38_88_170?.close()
        pipeline_50_33_50_154?.close()
        pipeline_36_51_50_167?.close()
    }
    fun load() {
        LogUtils.d("VTI", "[ VTITASK ] load")
        pipeline_36_51_50_167 = createTfLitePipelineWithConfig(context, Config.vtiConfig36)
        pipeline_38_38_88_170 = createTfLitePipelineWithConfig(context, Config.vtiConfig38)
        pipeline_50_33_50_154 = createTfLitePipelineWithConfig(context, Config.vtiConfig50)
    }

    fun process_38(image: ByteBuffer, kernel: ByteBuffer, outputPatch: ByteBuffer)
    {
        LogUtils.d("VTI", "[ VTITASK ] process 38")
        val pipeline_38 = pipeline_38_38_88_170 ?: return
        image.order(ByteOrder.nativeOrder())
        kernel.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline_38.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val kernelTensor = Tensor(pipeline_38.config.modelDescription.inputs[1].shape, kernel.asFloatBuffer())
        outputPatch.order(ByteOrder.nativeOrder())
        val outputPatchFloat = outputPatch.asFloatBuffer()
        outputPatchFloat.clear()
        val data = pipeline_38.process(imageTensor, kernelTensor)
        processTensorFlowOutput(outputPatchFloat, data[0].data, Config.vtiConfig38)
        outputPatchFloat.flip()
    }

    fun process_50(image: ByteBuffer, kernel: ByteBuffer, outputPatch: ByteBuffer)
    {
        LogUtils.d("VTI", "[ VTITASK ] process 50")
        val pipeline_50 = pipeline_50_33_50_154 ?: return
        image.order(ByteOrder.nativeOrder())
        kernel.order(ByteOrder.nativeOrder())
        outputPatch.order(ByteOrder.nativeOrder())
        val outputPatchFloat = outputPatch.asFloatBuffer()
        outputPatchFloat.clear()
        val imageTensor = Tensor(pipeline_50.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val kernelTensor = Tensor(pipeline_50.config.modelDescription.inputs[1].shape, kernel.asFloatBuffer())
        val data = pipeline_50.process(imageTensor, kernelTensor)
        processTensorFlowOutput(outputPatchFloat, data[0].data, Config.vtiConfig50)
        outputPatchFloat.flip()
    }

    fun process_36(image: ByteBuffer, kernel: ByteBuffer, outputPatch: ByteBuffer)
    {
        LogUtils.d("VTI", "[ VTITASK ] process 36")
        val pipeline_36 = pipeline_36_51_50_167 ?: return
        image.order(ByteOrder.nativeOrder())
        kernel.order(ByteOrder.nativeOrder())
        outputPatch.order(ByteOrder.nativeOrder())
        val outputPatchFloat = outputPatch.asFloatBuffer()
        outputPatchFloat.clear()
        val imageTensor = Tensor(pipeline_36.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val kernelTensor = Tensor(pipeline_36.config.modelDescription.inputs[1].shape, kernel.asFloatBuffer())
        val data = pipeline_36.process(imageTensor, kernelTensor)
        processTensorFlowOutput(outputPatchFloat, data[0].data, Config.vtiConfig36)
        outputPatchFloat.flip()
    }

    fun snpeMap(x: Int, y: Int, ch: Int) : Int
    {
        val CHSTRIDE = 1
        val XSTRIDE = 512
        val YSTRIDE = 2
        val linearIndexSNPE = x * XSTRIDE + y * YSTRIDE + ch * CHSTRIDE
        return linearIndexSNPE
    }
    fun tfliteMap(x: Int, y: Int, ch: Int) : Int
    {
        val XSTRIDE = 256
        val YSTRIDE = 1
        val CHSTRIDE = 65280
        val linearIndexTFLITE = x * XSTRIDE + y * YSTRIDE + ch * CHSTRIDE
        return linearIndexTFLITE
    }
    fun processTensorFlowOutput(outputBuffer: FloatBuffer, modelBuffer: FloatBuffer, config: Config)
    {
        val shape = config.modelDescription.outputs[0].shape
        for (n in 0 ..<shape[0]){
            for (h in 0..<shape[1] ){
                for (w in 0 ..< shape[2]){

                    outputBuffer.put(snpeMap(w,h,n), modelBuffer[tfliteMap(w,h,n)])
                }
            }
        }
    }

    companion object{
        fun createTfLitePipeline(context: Context): Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipelineFromConfig(Config.vtiConfig36, provider, context)
        }
        fun createTfLitePipelineWithConfig(context: Context, config: com.echonous.ai.vti.Config): com.echonous.ai.vti.Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipelineFromConfig(config, provider, context)
        }
    }
}