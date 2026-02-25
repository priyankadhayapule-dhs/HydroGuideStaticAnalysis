package com.echonous.thorsdk

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.echonous.ThorClip
import com.echonous.ai.Tensor
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.nio.charset.Charset

private const val INPUT_A4C = "BiplaneA4C.thor"
private const val INPUT_A2C = "BiplaneA2C.thor"

@RunWith(AndroidJUnit4::class)
class ClipReaderTest {

    private fun extractAssetToFile(name: String): String {
        val context = InstrumentationRegistry.getInstrumentation().context
        val inputStream = context.assets.open(name)
        val destination = File(context.cacheDir, name)
        val outputStream = destination.outputStream()
        inputStream.copyTo(outputStream)
        return destination.absolutePath
    }

    private fun saveImage(image: Tensor, file: File) {
        file.outputStream().buffered().use {
            val header = String.format("P5\n%d %d\n255\n", image.shape[3], image.shape[2])
            it.write(header.toByteArray(Charset.defaultCharset()))
            for (i in 0 until image.shape[2] * image.shape[3]) {
                val orig = image.data.get(i)
                val scaled = orig * 255.0
                val clamped = scaled.coerceIn(0.0, 255.0)
                val pixel = clamped.toInt()
                it.write(pixel)
            }
        }
    }

    private fun saveFrames(frames: List<Tensor>, folder: String) {
        val context = InstrumentationRegistry.getInstrumentation().context
        val destFolder = File(context.filesDir, folder)
        if (!destFolder.exists()) {
            destFolder.mkdirs()
        }
        frames.forEachIndexed() { index, frame ->
            saveImage(frame, File(destFolder, String.format("frame%03d.pgm", index)))
        }
    }

    @Test
    fun canReadClip() {
        val pathA4c = extractAssetToFile(INPUT_A4C)
        val frames = ThorClip.getScanConvertedBModeFrames(pathA4c, 224, 224)
        saveFrames(frames, INPUT_A4C)
        assertEquals(125, frames.size)
    }
}