import com.android.build.gradle.internal.tasks.factory.dependsOn
import java.io.ByteArrayOutputStream

// This is for dynamically adding the git commit hash to the version name
fun gitCommitHash(): String {
    return try {
        val stdout = ByteArrayOutputStream()
        project.exec {
            commandLine("git", "rev-parse", "--short", "HEAD")
            standardOutput = stdout
        }
        stdout.toString().trim()
    } catch (e: Exception) {
        "nogit"
    }
}

plugins {
    alias(libs.plugins.android.application)
    id("org.cyclonedx.bom") version "2.1.0"
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kapt)
    alias(libs.plugins.hilt) // Hilt plugin applied here (remove apply false)

}

android {
    namespace = "com.accessvascularinc.hydroguide.mma"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.accessvascularinc.hydroguide.mma"
        minSdk = 33
        targetSdk = 36
        versionCode = 1
        versionName = "1.0.git_${gitCommitHash()}" //dev version format: 0.1.0.git_5dee461 - App version is copied from here to strings.xml

        // Adding versionName to res/values/strings.xml for in-app access
        resValue(
            "string",
            "app_version",
            versionName!!
        )

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        
        // Specify which flavor to use from thorsdk library when dimension is missing
        missingDimensionStrategy("region", "basic")
        missingDimensionStrategy("ndkOption", "withTools")
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    buildFeatures {
        viewBinding = true
        dataBinding = true
    }

    lint {
        absolutePaths = false
    }

    project.tasks.preBuild.dependsOn("cyclonedxBom")
    kotlinOptions {
        jvmTarget = "17"
    }


}

dependencies {
   
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    implementation(libs.lifecycle.livedata.ktx)
    implementation(libs.lifecycle.viewmodel.ktx)
    implementation(libs.navigation.fragment)
    implementation(libs.navigation.ui)
    implementation(libs.legacy.support.v4)
    implementation(libs.recyclerview)
    implementation(libs.common)
    implementation(libs.preference)
    implementation(libs.androidplot)
    implementation(files("libs/d2xx.jar"))
    implementation(libs.core.ktx)

    implementation(libs.room.runtime)
    implementation(libs.lifecycle.runtime.ktx)
    implementation("androidx.security:security-crypto:1.1.0-alpha06")
    implementation("com.google.code.gson:gson:2.10.1")

    // Kotlin extensions + coroutines support
    implementation(libs.room.ktx)
    implementation(project(":thorsdk"))
    kapt(libs.room.compiler)
    // Hilt
    implementation(libs.hilt.android)
    kapt(libs.hilt.android.compiler)
    annotationProcessor(libs.hilt.android.compiler) // for Java

    // Retrofit, OkHttp, Gson
    implementation(libs.retrofit2)
    implementation(libs.retrofit2.converter.gson)
    implementation(libs.okhttp3)
    implementation(libs.okhttp3.logging.interceptor)
    implementation(libs.gson)

    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)


}

tasks.cyclonedxBom {
    setIncludeConfigs(
        listOf(
            "implementation",
            "runtimeClasspath",
            "debugImplementation",
            "releaseImplmementation",
            "debugRuntimeClasspath",
            "releaseRuntimeClasspath"
        )
    )
    setProjectType("application")
    setSchemaVersion("1.6")
    setDestination(project.file("build/outputs/"))
    setOutputName("sbom")
    setOutputFormat("json")
    setIncludeBomSerialNumber(false)
    setIncludeLicenseText(true)
    setComponentVersion("0.0.1")
}
