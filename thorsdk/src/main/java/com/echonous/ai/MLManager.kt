package com.echonous.ai

import android.content.Context
import com.echonous.hardware.ultrasound.ThorError
import com.echonous.hardware.ultrasound.UltrasoundManager
import java.io.File
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

class MLManager(private val context: Context) {

    init {
        nativeInit()
    }

    /**
     * Run verification tests on all thor files within the file tree given by [inputRoot].
     *
     * Results are written to a temporary folder under [Context.getCacheDir].
     */
    fun runVerification(appVersion: String, inputRoot: File, manager: UltrasoundManager, outputRoot: File = context.cacheDir): File {

        // Pair of (module name, callable to test that module)
        data class ModuleVerificationFunction(
            val name: String,
            val version: String,
            val verify: (manager: UltrasoundManager, input: String, outputRoot: String) -> Int
        )

        val modules = listOf(
            ModuleVerificationFunction("autolabel",         "unset", this::nativeAutolabelVerification),
            ModuleVerificationFunction("A4CA2C_guidance",   "unset", this::nativeA4CA2CGuidanceVerification),
            ModuleVerificationFunction("plax_guidance",     "unset", this::nativePLAXGuidanceVerification),
            ModuleVerificationFunction("FAST",              "unset", this::nativeFASTVerification),
           // ModuleVerificationFunction("preset",              "unset", this::nativePresetVerification),
            ModuleVerificationFunction("autodoppler_pw", "unset", this::nativeAutoDopplerPWVerification),
            ModuleVerificationFunction("autodoppler_tdi", "unset", this::nativeAutoDopplerTDIVerification),
            ModuleVerificationFunction("bladder", "unset", this::nativeBladderVerification)
        )
        //nativePresetAttach()
        // Using a custom time formatter because default will use ':' symbols, which don't play
        // nicely with certain file systems (cough NTFS)
        val formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd_HH-mm-ss")
        val now = LocalDateTime.now().format(formatter)

        // Get root output directory folder path. We do not actually create this folder here.
        // Instead, the entire output path for each .thor file is created using
        // [Files.createDirectories], which automatically creates all parent directories as needed.
        val datedOutputFolder = File(outputRoot, "verification_$now")

        // Attach CinePlayer as the CineProducer
        manager.ultrasoundPlayer.attachCine()

        try {
            // Iterate all defined modules, and look for a folder under inputRoot which matches their
            // name.
            for (module in modules) {
                val moduleInputDir = File(inputRoot, module.name)
                if (moduleInputDir.isDirectory) {
                    val moduleOutputDir = File(datedOutputFolder, module.name)
                    val task = VerificationTask(
                        moduleInputDir,
                        moduleOutputDir,
                        module.name,
                        appVersion,
                        module.version
                    )
                    task.performActionForAllInputs(manager.ultrasoundPlayer) { input, outputDirectory ->
                        ThorError.fromCode(module.verify(manager, input.path, outputDirectory.path))
                    }
                }
            }
            // EF Workflow
            val efModuleName = "ejection_fraction"
            val moduleInputDir = File(inputRoot, efModuleName)

            if (moduleInputDir.isDirectory) {
                val moduleOutputDir = File(datedOutputFolder, efModuleName)
                val task = VerificationTask(
                    moduleInputDir,
                    moduleOutputDir,
                    efModuleName,
                    appVersion,
                    "unset"
                )
                task.performActionForAllInputsContaining(
                    manager.ultrasoundPlayer,
                    "a4c"
                ) { input, outputDirectory ->
                    var inputA2C = input.path.replace("a4c", "a2c", true)
                    if(inputA2C.equals(input.path))
                    {
                        inputA2C = input.path.replace("A4C", "A2C", false)
                    }
                    ThorError.fromCode(
                        (this::nativeEFVerification)(
                            manager,
                            input.path,
                            inputA2C,
                            outputDirectory.path
                        )
                    )
                }
            }
        } finally {
            // Ensure cineplayer is always detached after tests, even if exception is thrown
            manager.ultrasoundPlayer.detachCine()
        }
        return datedOutputFolder
    }

    private external fun nativeAutolabelVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativeAutoDopplerPWVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativeAutoDopplerTDIVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativeA4CA2CGuidanceVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativePLAXGuidanceVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativeFASTVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativePresetVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    private external fun nativeEFVerification(manager: UltrasoundManager, inputA4C: String, inputA2C: String, outputRoot: String): Int
    private external fun nativeBladderVerification(manager: UltrasoundManager, input: String, outputRoot: String): Int
    companion object {
        @JvmStatic
        private external fun nativeInit()
    }
}