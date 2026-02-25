package com.echonous.thorsdk

import android.content.res.AssetManager
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.echonous.ThorClip
import com.echonous.ai.MLModel
import com.echonous.ai.Tensor
import com.echonous.ai.autolabel.Config
import com.echonous.ai.tflite.TfLiteProvider
import com.google.android.gms.tasks.Tasks
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import java.nio.charset.Charset

private const val INPUT_A4C = "BiplaneA4C.thor"
private const val INPUT_A2C = "BiplaneA2C.thor"

@RunWith(AndroidJUnit4::class)
class ModelRunTest {

    private fun runModelOnFolder(model: MLModel, inputFolder: File, outputFolder: File) {
        if (!outputFolder.exists()) {
            outputFolder.mkdirs()
        }
        val files = inputFolder.listFiles()
    }

    private fun runModelOnFile(model: MLModel, input: File, outputFolder: File) {
        require(model.description.inputs.size == 1) { "Model must have a single input" }
        val inputShape = model.description.inputs[0].shape
        require(inputShape.size == 4) { "Input shape must be 4D" }
        require(inputShape[0] == 1) { "Input shape must have batch size 1" }
        require(inputShape[1] == 1) { "Input shape must have channel size 1" }
        val height = inputShape[2]
        val width = inputShape[3]

        // Read input clip
        val frames = ThorClip.getScanConvertedBModeFrames(input.name, width, height)

        // Prepare buffers for output(s)
        val outputBuffers = model.description.outputs.map { description ->
            val singleOutputSize = description.shape.fold(1, Int::times)
            ByteBuffer.allocate(frames.size * singleOutputSize).also { it.order(ByteOrder.LITTLE_ENDIAN) }
        }
        val floatBuffers = outputBuffers.map(ByteBuffer::asFloatBuffer)

        // Run model on each frame, storing outputs in buffers
        frames.map { frame ->
            model.predict(listOf(frame)).use {
                it.forEachIndexed { index, output ->
                    floatBuffers[index].put(output.data)
                }
            }
        }

        // Save output buffers to raw files
        outputBuffers.forEachIndexed { index, buffer ->
            // Note the bytebuffers position & limit are independent from the floatbuffers, so we don't need to flip
            val output = File(outputFolder, String.format("%s.raw", model.description.outputs[index].name))
            output.outputStream().channel.write(buffer)
        }
    }

    private fun extractAssetToFile(assets: AssetManager, assetFile: File, outputDir: File): File {
        val inputStream = assets.open(assetFile.path)
        if (!outputDir.exists()) {
            outputDir.mkdirs()
        }
        val destination = File(outputDir, assetFile.name)
        val outputStream = destination.outputStream()
        inputStream.copyTo(outputStream)
        return destination
    }

    private fun extractAssetsToFilesystem(assetFolder: String): File {
        val context = InstrumentationRegistry.getInstrumentation().context
        val destinationFolder = File(context.filesDir, assetFolder)
        context.assets.list(assetFolder)?.forEach {
            val assetName = File(assetFolder, it)
            extractAssetToFile(context.assets, assetName, destinationFolder)
        }
        return destinationFolder
    }

    @Test
    fun runAutolabel() {
//        val config = Config.version_3_6
//        val context = InstrumentationRegistry.getInstrumentation().context
//        val task = TfLiteProvider.createGMSProvider(context).continueWith { provider ->
//            provider.result.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
//        }.continueWith { model ->
//            val inputFolder = File(context.cacheDir, "ModelRunTestInputs")
//            val outputFolder = File(context.filesDir, "ModelRunTestOutputs")
//            if (!outputFolder.exists()) {
//                outputFolder.mkdirs()
//            }
//            extractAssetToFile(context.assets, File(INPUT_A4C), inputFolder)
//            extractAssetToFile(context.assets, File(INPUT_A2C), inputFolder)
//            runModelOnFolder(model.result, inputFolder, outputFolder)
//        }
//        Tasks.await(task)
    }
}