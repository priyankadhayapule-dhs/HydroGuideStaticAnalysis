package com.echonous.ai.autolabel

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
    val anchors: List<AnchorDefinition>,
    val labels: List<LabelDefinition>,
    val views: List<String>,
    val viewThreshold: Float,
    val labelBufferFraction: Float,
    val smoothingFrames: Int,
    val viewLabelMatrix: BooleanArray,
) {

    val yoloPredictionSize get() = 5 + labels.size
    val columnPixels get() = imageWidth.toFloat() / gridWidth.toFloat()
    val rowPixels get() = imageHeight.toFloat() / gridHeight.toFloat()

    constructor(
        modelName: String,
        modelDescription: MLModel.Description,
        imageWidth: Int,
        imageHeight: Int,
        gridWidth: Int,
        gridHeight: Int,
        anchors: List<AnchorDefinition>,
        labels: List<LabelDefinition>,
        views: List<String>,
        viewThreshold: Float,
        labelBufferFraction: Float,
        smoothingFrames: Int,
        labelsInViews: Map<String, List<String>>,
    ) : this(
        modelName = modelName,
        modelDescription = modelDescription,
        imageWidth = imageWidth,
        imageHeight = imageHeight,
        gridWidth = gridWidth,
        gridHeight = gridHeight,
        anchors = anchors,
        labels = labels,
        views = views,
        viewThreshold = viewThreshold,
        labelBufferFraction = labelBufferFraction,
        smoothingFrames = smoothingFrames,
        viewLabelMatrix = makeViewLabelMatrix(labels, views, labelsInViews)
    )

    fun isLabelInView(label: Int, view: Int): Boolean {
        if (label < 0 || label >= labels.size) { return false }
        if (view < 0 || view >= views.size) { return false }
        return viewLabelMatrix[view*labels.size + label]
    }

    @Serializable
    data class AnchorDefinition(
        val height: Float,
        val width: Float
    )

    @Serializable
    data class LabelDefinition(
        val name: String,
        val threshold: Float,
    )

    companion object {
        val version_3_6 = Config(
            modelName = "autolabel_v36_prelu.tflite",
            modelDescription = MLModel.Description(
                name = "AutoLabelV3.6",
                version = "3.6",
                inputs = listOf(MLModel.Feature(name = "image", shape = intArrayOf(1, 1, 224, 224))),
                outputs = listOf(
                    MLModel.Feature(name = "yolo", shape = intArrayOf(1, 75, 14, 14)),
                    MLModel.Feature(name = "view", shape = intArrayOf(1, 20)),
                ),
            ),
            imageWidth = 224,
            imageHeight = 224,
            gridWidth = 14,
            gridHeight = 14,
            anchors = listOf(
                AnchorDefinition(32.0f, 16.0f),
                AnchorDefinition(20.0f, 30.0f),
                AnchorDefinition(58.0f, 33.0f),
            ),
            labels = listOf(
                LabelDefinition(name="AO",      threshold= 0.3f),
                LabelDefinition(name="AL PAP",  threshold= 0.4f),
                LabelDefinition(name="AO ARCH", threshold= 0.4f),
                LabelDefinition(name="AV",      threshold= 0.3f),
                LabelDefinition(name="DA",      threshold= 0.4f),
                LabelDefinition(name="IAS",     threshold= 0.4f),
                LabelDefinition(name="IVC",     threshold= 0.4f),
                LabelDefinition(name="IVS",     threshold= 0.4f),
                LabelDefinition(name="LA",      threshold= 0.4f),
                LabelDefinition(name="LIVER",   threshold= 0.4f),
                LabelDefinition(name="LV",      threshold= 0.4f),
                LabelDefinition(name="LVOT",    threshold= 0.4f),
                LabelDefinition(name="MPA",     threshold= 0.3f),
                LabelDefinition(name="MV",      threshold= 0.4f),
                LabelDefinition(name="PM PAP",  threshold= 0.4f),
                LabelDefinition(name="PV",      threshold= 0.3f),
                LabelDefinition(name="RA",      threshold= 0.4f),
                LabelDefinition(name="RV",      threshold= 0.4f),
                LabelDefinition(name="RVOT",    threshold= 0.4f),
                LabelDefinition(name="TV",      threshold= 0.4f),
            ),
            views = listOf(
                "A2C-OPTIMAL",
                "A2C-SUBVIEW",
                "A3C-OPTIMAL",
                "A3C-SUBVIEW",
                "A4C-OPTIMAL",
                "A4C-SUBVIEW",
                "A5C",
                "PLAX-OPTIMAL",
                "PLAX-SUBVIEW",
                "RVOT",
                "RVIT",
                "PSAX-AV",
                "PSAX-MV",
                "PSAX-PM",
                "PSAX-AP",
                "SUBCOSTAL-4C-OPTIMAL",
                "SUBCOSTAL-4C-SUBVIEW",
                "SUBCOSTAL-IVC",
                "SUPRASTERNAL",
                "OTHER",
            ),
            viewThreshold = 0.4f,
            labelBufferFraction = 0.4f,
            smoothingFrames = 15,
            labelsInViews = mapOf(
                "A2C-OPTIMAL" to listOf("LA", "LV", "MV"),
                "A2C-SUBVIEW" to listOf("LA", "LV", "MV"),
                "A3C-OPTIMAL" to listOf("AO", "AV", "LA", "LV", "LVOT", "MV"),
                "A3C-SUBVIEW" to listOf("AO", "AV", "LA", "LV", "LVOT", "MV"),
                "A4C-OPTIMAL" to listOf("IAS", "IVS", "LA", "LV", "MV", "RA", "RV", "TV"),
                "A4C-SUBVIEW" to listOf("AO", "AV", "IAS", "IVS", "LA", "LV", "LVOT", "MV", "RA", "RV", "TV"),
                "A5C" to listOf("AO", "AV", "IAS", "IVS", "LA", "LV", "LVOT", "MV", "RA", "RV", "TV"),
                "PLAX-OPTIMAL" to listOf("AO", "AV", "IVS", "LA", "LV", "LVOT", "MV", "RV"),
                "PLAX-SUBVIEW" to listOf("AO", "AV", "IVS", "LA", "LV", "LVOT", "MV", "RV", "MPA", "PV", "IVC", "RA", "TV"),
                "RVOT" to listOf("IVS", "LV", "MPA", "PV", "RVOT"),
                "RVIT" to listOf("IVC", "IVS", "LV", "RA", "RV", "TV"),
                "PSAX-AV" to listOf("AV", "LA", "MPA", "PV", "RA", "RVOT", "TV"),
                "PSAX-MV" to listOf("IVS", "LV", "MV", "RV"),
                "PSAX-PM" to listOf("AL PAP", "IVS", "LV", "PM PAP", "RV"),
                "PSAX-AP" to listOf("IVS", "LV", "RV"),
                "SUBCOSTAL-4C-OPTIMAL" to listOf("IAS", "IVS", "LA", "LIVER", "LV", "MV", "RA", "RV", "TV"),
                "SUBCOSTAL-4C-SUBVIEW" to listOf("IAS", "IVS", "LA", "LIVER", "LV", "MV", "RA", "RV", "TV"),
                "SUBCOSTAL-IVC" to listOf("IVC", "LIVER"),
                "SUPRASTERNAL" to listOf("AO ARCH", "DA"),
                "OTHER" to listOf(),
            )
        )

        val version_4_0 = Config(
            modelName = "autolabel_pw.tflite",
            modelDescription = MLModel.Description(
                name = "AutoLabelV4.0",
                version = "4.0",
                inputs = listOf(MLModel.Feature(name = "image", shape = intArrayOf(1, 224, 224))),
                outputs = listOf(
                    MLModel.Feature(name = "yolo", shape = intArrayOf(1, 75, 14, 14)),
                    MLModel.Feature(name = "view", shape = intArrayOf(1, 20)),
                ),
            ),
            imageWidth = 224,
            imageHeight = 224,
            gridWidth = 14,
            gridHeight = 14,
            anchors = listOf(
                AnchorDefinition(32.0f, 16.0f),
                AnchorDefinition(20.0f, 30.0f),
                AnchorDefinition(58.0f, 33.0f),
            ),
            labels = listOf(
                LabelDefinition(name="AO",      threshold= 0.3f),
                LabelDefinition(name="AL PAP",  threshold= 0.4f),
                LabelDefinition(name="AO ARCH", threshold= 0.4f),
                LabelDefinition(name="AV",      threshold= 0.3f),
                LabelDefinition(name="DA",      threshold= 0.4f),
                LabelDefinition(name="IAS",     threshold= 0.4f),
                LabelDefinition(name="IVC",     threshold= 0.4f),
                LabelDefinition(name="IVS",     threshold= 0.4f),
                LabelDefinition(name="LA",      threshold= 0.4f),
                LabelDefinition(name="LIVER",   threshold= 0.4f),
                LabelDefinition(name="LV",      threshold= 0.4f),
                LabelDefinition(name="LVOT",    threshold= 0.2f),
                LabelDefinition(name="MPA",     threshold= 0.3f),
                LabelDefinition(name="MV",      threshold= 0.4f),
                LabelDefinition(name="PM PAP",  threshold= 0.4f),
                LabelDefinition(name="PV",      threshold= 0.3f),
                LabelDefinition(name="RA",      threshold= 0.4f),
                LabelDefinition(name="RV",      threshold= 0.4f),
                LabelDefinition(name="RVOT",    threshold= 0.4f),
                LabelDefinition(name="TV",      threshold= 0.4f),
            ),
            views = listOf(
                "A2C-OPTIMAL",
                "A2C-SUBVIEW",
                "A3C-OPTIMAL",
                "A3C-SUBVIEW",
                "A4C-OPTIMAL",
                "A4C-SUBVIEW",
                "A5C",
                "PLAX-OPTIMAL",
                "PLAX-SUBVIEW",
                "RVOT",
                "RVIT",
                "PSAX-AV",
                "PSAX-MV",
                "PSAX-PM",
                "PSAX-AP",
                "SUBCOSTAL-4C-OPTIMAL",
                "SUBCOSTAL-4C-SUBVIEW",
                "SUBCOSTAL-IVC",
                "SUPRASTERNAL",
                "OTHER",
            ),
            viewThreshold = 0.4f,
            labelBufferFraction = 0.4f,
            smoothingFrames = 15,
            labelsInViews = mapOf(
                "A2C-OPTIMAL" to listOf("LA", "LV", "MV"),
                "A2C-SUBVIEW" to listOf("LA", "LV", "MV"),
                "A3C-OPTIMAL" to listOf("AO", "AV", "LA", "LV", "LVOT", "MV"),
                "A3C-SUBVIEW" to listOf("AO", "AV", "LA", "LV", "LVOT", "MV"),
                "A4C-OPTIMAL" to listOf("IAS", "IVS", "LA", "LV", "MV", "RA", "RV", "TV"),
                "A4C-SUBVIEW" to listOf("AO", "AV", "IAS", "IVS", "LA", "LV", "LVOT", "MV", "RA", "RV", "TV"),
                "A5C" to listOf("AO", "AV", "IAS", "IVS", "LA", "LV", "LVOT", "MV", "RA", "RV", "TV"),
                "PLAX-OPTIMAL" to listOf("AO", "AV", "IVS", "LA", "LV", "LVOT", "MV", "RV"),
                "PLAX-SUBVIEW" to listOf("AO", "AV", "IVS", "LA", "LV", "LVOT", "MV", "RV", "MPA", "PV", "IVC", "RA", "TV"),
                "RVOT" to listOf("IVS", "LV", "MPA", "PV", "RVOT"),
                "RVIT" to listOf("IVC", "IVS", "LV", "RA", "RV", "TV"),
                "PSAX-AV" to listOf("AV", "LA", "MPA", "PV", "RA", "RVOT", "TV"),
                "PSAX-MV" to listOf("IVS", "LV", "MV", "RV"),
                "PSAX-PM" to listOf("AL PAP", "IVS", "LV", "PM PAP", "RV"),
                "PSAX-AP" to listOf("IVS", "LV", "RV"),
                "SUBCOSTAL-4C-OPTIMAL" to listOf("IAS", "IVS", "LA", "LIVER", "LV", "MV", "RA", "RV", "TV"),
                "SUBCOSTAL-4C-SUBVIEW" to listOf("IAS", "IVS", "LA", "LIVER", "LV", "MV", "RA", "RV", "TV"),
                "SUBCOSTAL-IVC" to listOf("IVC", "LIVER"),
                "SUPRASTERNAL" to listOf("AO ARCH", "DA"),
                "OTHER" to listOf(),
            )
        )

        val version_tdi = Config(
            modelName = "autolabel_tdi.tflite",
            modelDescription = MLModel.Description(
                name = "AutoLabelTDI",
                version = "1.0",
                inputs = listOf(MLModel.Feature(name = "image", shape = intArrayOf(1, 224, 224))),
                outputs = listOf(
                    MLModel.Feature(name = "yolo", shape = intArrayOf(1, 24, 14, 14)),
                    MLModel.Feature(name = "view", shape = intArrayOf(1, 20)),
                ),
            ),
            imageWidth = 224,
            imageHeight = 224,
            gridWidth = 14,
            gridHeight = 14,
            anchors = listOf(
                AnchorDefinition(32.0f, 16.0f),
                AnchorDefinition(20.0f, 30.0f),
                AnchorDefinition(58.0f, 33.0f),
            ),
            labels = listOf(
                LabelDefinition(name="MVLA", threshold=0.2f),
                LabelDefinition(name="MVSA", threshold=0.2f),
                LabelDefinition(name="TVLA", threshold=0.2f),
            ),
            views = listOf(
                "A2C-OPTIMAL",
                "A2C-SUBVIEW",
                "A3C-OPTIMAL",
                "A3C-SUBVIEW",
                "A4C-OPTIMAL",
                "A4C-SUBVIEW",
                "A5C",
                "PLAX-OPTIMAL",
                "PLAX-SUBVIEW",
                "RVOT",
                "RVIT",
                "PSAX-AV",
                "PSAX-MV",
                "PSAX-PM",
                "PSAX-AP",
                "SUBCOSTAL-4C-OPTIMAL",
                "SUBCOSTAL-4C-SUBVIEW",
                "SUBCOSTAL-IVC",
                "SUPRASTERNAL",
                "OTHER",
            ),
            viewThreshold = 0.4f,
            labelBufferFraction = 0.4f,
            smoothingFrames = 15,
            labelsInViews = mapOf(
                "A2C-OPTIMAL" to listOf(),
                "A2C-SUBVIEW" to listOf(),
                "A3C-OPTIMAL" to listOf(),
                "A3C-SUBVIEW" to listOf(),
                "A4C-OPTIMAL" to listOf("MVLA", "MVSA", "TVLA"),
                "A4C-SUBVIEW" to listOf("MVLA", "MVSA", "TVLA"),
                "A5C" to listOf("MVLA", "MVSA", "TVLA"),
                "PLAX-OPTIMAL" to listOf(),
                "PLAX-SUBVIEW" to listOf(),
                "RVOT" to listOf(),
                "RVIT" to listOf(),
                "PSAX-AV" to listOf(),
                "PSAX-MV" to listOf(),
                "PSAX-PM" to listOf(),
                "PSAX-AP" to listOf(),
                "SUBCOSTAL-4C-OPTIMAL" to listOf(),
                "SUBCOSTAL-4C-SUBVIEW" to listOf(),
                "SUBCOSTAL-IVC" to listOf(),
                "SUPRASTERNAL" to listOf(),
                "OTHER" to listOf()
            ),
        )

        fun makeViewLabelMatrix(labels: List<LabelDefinition>, views: List<String>, labelsInViews: Map<String, List<String>>): BooleanArray {
            val matrix = BooleanArray(views.size * labels.size)
            for ((viewIndex, view) in views.withIndex()) {
                for ((labelIndex, label) in labels.withIndex()) {
                    matrix[viewIndex * labels.size + labelIndex] = (labelsInViews[view]?.contains(label.name) == true)
                }
            }
            return matrix
        }
    }

    override fun equals(other: Any?): Boolean {
        // Auto generated equals method required because of FloatArray property
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as Config

        if (modelName != other.modelName) return false
        if (imageWidth != other.imageWidth) return false
        if (imageHeight != other.imageHeight) return false
        if (gridWidth != other.gridWidth) return false
        if (gridHeight != other.gridHeight) return false
        if (anchors != other.anchors) return false
        if (labels != other.labels) return false
        if (views != other.views) return false
        if (viewThreshold != other.viewThreshold) return false
        if (labelBufferFraction != other.labelBufferFraction) return false
        if (smoothingFrames != other.smoothingFrames) return false
        if (!viewLabelMatrix.contentEquals(other.viewLabelMatrix)) return false

        return true
    }

    override fun hashCode(): Int {
        // Auto generated hashCode method required because of FloatArray property
        var result = modelName.hashCode()
        result = 31 * result + imageWidth
        result = 31 * result + imageHeight
        result = 31 * result + gridWidth
        result = 31 * result + gridHeight
        result = 31 * result + anchors.hashCode()
        result = 31 * result + labels.hashCode()
        result = 31 * result + views.hashCode()
        result = 31 * result + viewThreshold.hashCode()
        result = 31 * result + labelBufferFraction.hashCode()
        result = 31 * result + smoothingFrames
        result = 31 * result + viewLabelMatrix.contentHashCode()
        return result
    }
}