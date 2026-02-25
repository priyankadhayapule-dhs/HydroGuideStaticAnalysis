package com.echonous.ai.fast

import com.echonous.ai.MLModel
import kotlinx.serialization.Serializable

@Serializable
data class Config(
    val modelName: String,
    val modelDescription: MLModel.Description,
    val imageWidth: Int,
    val imageHeight: Int,
    val gridWidth: Int,
    val gridHeight: Int,
    val anchorCount: Int,
    val inferenceIntervalFrames: Int,
    val smoothingFramesCount: Int,
    val developerMode: Boolean,
    val viewNames: List<String>,
    val displayNames: List<String>,
    val labelNamesWithView: List<String>,
    val labelNames: List<String>
) {
    val yoloPredictionSize get() = 5 + labelNames.size
    companion object {
        val fast_142704= Config(
            modelName = "FASTExam/20230208-142704-yoloabdv1b-v44-irb.tflite",
            modelDescription = MLModel.Description(
                name = "FAST 142704",
                version = "2023-02-08",
                inputs = listOf(MLModel.Feature(name="input_frames", shape = intArrayOf(1,128,128))),
                outputs = listOf(
                    MLModel.Feature(name="yolo", shape = intArrayOf(1,66,8,8)),
                    MLModel.Feature(name="view", shape=intArrayOf(15))
                )
            ),
            imageWidth = 128,
            imageHeight = 128,
            gridWidth = 8,
            gridHeight = 8,
            anchorCount = 3,
            inferenceIntervalFrames = 2,
            smoothingFramesCount = 16,
            developerMode = false,
            viewNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "SUB", "SUB2", "A4C", "A2C", "PLAX", "PSAX", "Lung"),
            displayNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Lung"),
            labelNamesWithView = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto"),
            labelNames = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto")
        )

        val fast_142730= Config(
            modelName = "FASTExam/20230208-142730-yoloabdv1b-v44-irb.tflite",
            modelDescription = MLModel.Description(
                name = "FAST 142730",
                version = "2023-02-08",
                inputs = listOf(MLModel.Feature(name="input_frames", shape = intArrayOf(1,128,128))),
                outputs = listOf(
                    MLModel.Feature(name="yolo", shape = intArrayOf(1,66,8,8)),
                    MLModel.Feature(name="view", shape=intArrayOf(15))
                )
            ),
            imageWidth = 128,
            imageHeight = 128,
            gridWidth = 8,
            gridHeight = 8,
            anchorCount = 3,
            inferenceIntervalFrames = 2,
            smoothingFramesCount = 16,
            developerMode = false,
            viewNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "SUB", "SUB2", "A4C", "A2C", "PLAX", "PSAX", "Lung"),
            displayNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Lung"),
            labelNamesWithView = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto"),
            labelNames = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto")
        )

        val fast_142755= Config(
            modelName = "FASTExam/20230208-142755-yoloabdv1b-v44-irb.tflite",
            modelDescription = MLModel.Description(
                name = "FAST 142755",
                version = "2023-02-08",
                inputs = listOf(MLModel.Feature(name="input_frames", shape = intArrayOf(1,128,128))),
                outputs = listOf(
                    MLModel.Feature(name="yolo", shape = intArrayOf(1,66,8,8)),
                    MLModel.Feature(name="view", shape=intArrayOf(15))
                )
            ),
            imageWidth = 128,
            imageHeight = 128,
            gridWidth = 8,
            gridHeight = 8,
            anchorCount = 3,
            inferenceIntervalFrames = 2,
            smoothingFramesCount = 16,
            developerMode = false,
            viewNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "SUB", "SUB2", "A4C", "A2C", "PLAX", "PSAX", "Lung"),
            displayNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Lung"),
            labelNamesWithView = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto"),
            labelNames = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto")
        )

        val fast_142805= Config(
            modelName = "FASTExam/20230208-142805-yoloabdv1b-v44-irb.tflite",
            modelDescription = MLModel.Description(
                name = "FAST 142805",
                version = "2023-02-08",
                inputs = listOf(MLModel.Feature(name="input_frames", shape = intArrayOf(1,128,128))),
                outputs = listOf(
                    MLModel.Feature(name="yolo", shape = intArrayOf(1,66,8,8)),
                    MLModel.Feature(name="view", shape=intArrayOf(15))
                )
            ),
            imageWidth = 128,
            imageHeight = 128,
            gridWidth = 8,
            gridHeight = 8,
            anchorCount = 3,
            inferenceIntervalFrames = 2,
            smoothingFramesCount = 16,
            developerMode = false,
            viewNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "SUB", "SUB2", "A4C", "A2C", "PLAX", "PSAX", "Lung"),
            displayNames = listOf("empty", "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Lung"),
            labelNamesWithView = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto"),
            labelNames = listOf("liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto")
        )
    }
}