package com.echonous.ai

import android.os.Build
import com.echonous.hardware.ultrasound.ThorError
import com.echonous.hardware.ultrasound.UltrasoundPlayer
import com.echonous.hardware.ultrasound.andThen
import kotlinx.serialization.ExperimentalSerializationApi
import kotlinx.serialization.KSerializer
import kotlinx.serialization.Serializable
import kotlinx.serialization.descriptors.PrimitiveKind
import kotlinx.serialization.descriptors.PrimitiveSerialDescriptor
import kotlinx.serialization.descriptors.SerialDescriptor
import kotlinx.serialization.encoding.Decoder
import kotlinx.serialization.encoding.Encoder
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.encodeToStream
import java.io.File
import java.io.FileOutputStream
import java.nio.file.Files
import java.time.ZonedDateTime
import java.util.Locale

class VerificationTask(
    val inputRoot: File,
    val outputRoot: File,
    val moduleName: String,
    val appVersion: String,
    val moduleVersion: String,
) {

    init {
        Files.createDirectories(outputRoot.toPath())

        require(inputRoot.isAbsolute) { "inputRoot must be an absolute path (given \"$inputRoot\")" }
        require(inputRoot.isDirectory) { "inputRoot must be a directory (given \"$inputRoot\")" }
        require(outputRoot.isAbsolute) { "outputRoot must be an absolute path (given \"$outputRoot\")" }
        require(outputRoot.isDirectory) { "outputRoot must be a directory (given \"$outputRoot\")" }
    }

    val inputs: List<File> = findThorFiles(inputRoot)

    /**
     * Run an action for every thor file in this task. The action receives the full path to the
     * input thor file, as well as an output directory in which to write results.
     * under \c outputRoot.
     *
     * Creates a metadata file in outputRoot/meta.json, containing information about the entire task
     * invocation (see TaskMetadata class).
     */
    @OptIn(ExperimentalSerializationApi::class)
    fun performActionForAllInputs(
        player: UltrasoundPlayer?,
        action: (input: File, outputDirectory: File) -> ThorError)
    {
        val startedAt = ZonedDateTime.now()
        for (input in inputs) {
            performActionForInput(input, player, action)
        }
        val completedAt = ZonedDateTime.now()
        val device = getDeviceName()
        val metadata = TaskMetadata(moduleName, appVersion, moduleVersion, device, startedAt, completedAt)
        val metadataOutputStream = FileOutputStream(File(outputRoot, "meta.json"))
        Json.encodeToStream(metadata, metadataOutputStream)
    }

    /**
     * Run an action for every thor file that contains the filter in the filename in this task. The
     * action receives the full path to the input thor file, as well as an output directory in which
     * to write results under \c outputRoot.
     *
     * Creates a metadata file in outputRoot/meta.json, containing information about the entire task
     * invocation (see TaskMetadata class).
     */
    @OptIn(ExperimentalSerializationApi::class)
    fun performActionForAllInputsContaining(
            player: UltrasoundPlayer?,
            filter: String,
            action: (input: File, outputDirectory: File) -> ThorError)
    {
        val startedAt = ZonedDateTime.now()
        for (input in inputs) {
            if(input.path.contains(filter, true)) {
                performActionForInput(input, player, action)
            }
        }
        val completedAt = ZonedDateTime.now()
        val device = getDeviceName()
        val metadata = TaskMetadata(moduleName, appVersion, moduleVersion, device, startedAt, completedAt)
        val metadataOutputStream = FileOutputStream(File(outputRoot, "meta.json"))
        Json.encodeToStream(metadata, metadataOutputStream)
    }

    /**
     * Run an action for a single thor file, which must exist under \c inputRoot. The action
     * receives the full path to the input thor file, as well as an output directory in which to
     * write results.
     *
     * Creates a metadata file in outputDirectory/meta.json, containing information about the action
     * invocation (see FileMetadata class).
     */
    @OptIn(ExperimentalSerializationApi::class)
    fun performActionForInput(
        input: File,
        player: UltrasoundPlayer?,
        action: (input: File, outputDirectory: File) -> ThorError)
    {
        data class Result(val code: ThorError, val message: String = code.toString())

        // Possible future enhancement:
        //  check for existing metadata, only re-run if input file is newer or if there was an error?

        val outputDirectory = createOutputDirectoryMatching(input)
        val startedAt = ZonedDateTime.now()
        val (code, message) = try {
            // if we were given an UltrasoundPlayer, open the file
            val openCode = player?.openRawFile(input.path) ?: ThorError.THOR_OK
            // Run action if openCode is ok
            Result(openCode.andThen { action(input, outputDirectory) })
        } catch (e: Error) {
            // Catch exceptions thrown by either action or UltrasoundPlayer.openRawFile
            Result(ThorError.THOR_ERROR, "Exception thrown: $e")
        }
        val completedAt = ZonedDateTime.now()

        // save metadata to output folder
        val metadata = FileMetadata(input.name, startedAt, completedAt, code, message)
        FileOutputStream(File(outputDirectory, "meta.json")).use { outputStream ->
            Json.encodeToStream(metadata, outputStream)
        }
    }

    /**
     * Create and returns an output directory (under outputRoot) for a given input file (which must
     * be somewhere under inputRoot). Intermediate directories in the output tree are created as
     * needed.
     */
    fun createOutputDirectoryMatching(input: File): File {
        // Comments show an example, given:
        //  inputRoot = "/path/to/inputs"
        //  inputFile = "/path/to/inputs/day1/batch1/foo.thor"
        //  outputRoot = "/path/to/outputs"
        val relativeParent = parentDirectoryRelativeTo(input, inputRoot)
        // relativeParent = "day1/batch1"
        val relativeDirectory = File(relativeParent, input.nameWithoutExtension)
        // relativeDirectory = "day1/batch1/foo"
        val outputDirectory = File(outputRoot, relativeDirectory.path)
        // outputDirectory = "/path/to/outputs/day1/batch1/foo"

        Files.createDirectories(outputDirectory.toPath())
        check(outputDirectory.isDirectory)

        return outputDirectory
    }

    /**
     * Metadata about the entire task run.
     */
    @Serializable
    data class TaskMetadata(
        val moduleName: String,
        val appVersion: String,
        val moduleVersion: String,
        val device: String,
        @Serializable(with=ISO8601Serializer::class)
        val startedAt: ZonedDateTime,
        @Serializable(with=ISO8601Serializer::class)
        val completedAt: ZonedDateTime,
    )

    /**
     * Metadata about a single file test run.
     */
    @Serializable
    data class FileMetadata(
        var name: String,
        @Serializable(with=ISO8601Serializer::class)
        val startedAt: ZonedDateTime,
        @Serializable(with=ISO8601Serializer::class)
        var completedAt: ZonedDateTime,
        var resultCode: ThorError,
        var resultMessage: String
    )

    object ISO8601Serializer : KSerializer<ZonedDateTime> {
        override val descriptor: SerialDescriptor = PrimitiveSerialDescriptor("ZonedDateTime", PrimitiveKind.STRING)
        override fun serialize(encoder: Encoder, value: ZonedDateTime) {
            val string = value.toString()
            encoder.encodeString(string)
        }
        override fun deserialize(decoder: Decoder): ZonedDateTime {
            val string = decoder.decodeString()
            return ZonedDateTime.parse(string)
        }
    }

    companion object {
        /**
         * Find all files ending in ".thor" in a given file tree
         */
        fun findThorFiles(root: File): List<File> {
            return root.walk().filter {
                it.isFile && it.extension == "thor"
            }.toList()
        }

        /**
         * Get the parent directory of \c file, relative to \c root
         * root must be a superparent of file
         */
        fun parentDirectoryRelativeTo(file: File, root: File): File {
            val parent = file.parentFile
            require(parent != null) { "No parent directory found for file $file" }
            return parent.relativeTo(root) // throws IllegalArgumentException if root is not superparent
        }

        /**
         * Get the (hopefully human readable) name of the device, reading from the information in
         * android.os.Build.
         */
        fun getDeviceName(): String {
            // Code adapted from https://stackoverflow.com/questions/1995439/get-android-phone-model-programmatically-how-to-get-device-name-and-model-prog/26117427#26117427
            fun capitalize(text: String): String {
                // Note: using hard coded US locale because this is not user facing, but for testing
                // purposes
                return text.replaceFirstChar { if (it.isLowerCase()) it.titlecase(Locale.US) else it.toString() }
            }
            val manufacturer = Build.MANUFACTURER
            val model = Build.MODEL
            return if (model.startsWith(manufacturer, ignoreCase = true)) {
                capitalize(model)
            } else {
                capitalize("$manufacturer $model")
            }
        }
    }
}
