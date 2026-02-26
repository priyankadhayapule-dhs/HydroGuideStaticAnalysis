// Top-level build file where you can add configuration options common to all sub-projects/modules.
plugins {
    alias(libs.plugins.android.application) apply false
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.android.dynamic.feature) apply false
    alias(libs.plugins.kotlin.android) apply false
    alias(libs.plugins.kotlin.compose) apply false
    alias(libs.plugins.ksp) apply false
    alias(libs.plugins.hilt) apply false
    alias(libs.plugins.gms.service) apply false
    alias(libs.plugins.firebase.crashlytics) apply false
    alias(libs.plugins.firebase.app.distribution) apply false
    id("org.sonarqube") version "7.1.0.6387"

}

// This block encapsulates custom properties and makes them available to all
// modules in the project.
ext {
    // The following are only a few examples of the types of properties you can define.
    set("compileSdkVersion", 36)
    set("targetSdkVersion", 36)
    set("minSdkVersion", 33)
    set("versionCode", 70014)
    set("appVersion", "7.0.0")
    set("buildNumber", "14")
    set("enableLint", false)
    //Lint options
    // if you prefer, you can continue to check for errors in release builds,
    set("lintCheckReleaseBuilds", extra["enableLint"])
    // set to true to turn off analysis progress reporting by lint
    set("lintQuiet", !(extra["enableLint"] as Boolean))
    // if true, stop the gradle build if errors are found
    set("lintAbortOnError", extra["enableLint"])
    // if true, only report errors
    set("lintIgnoreWarnings", !(extra["enableLint"] as Boolean))
}


sonarqube {
    properties {
        property("sonar.projectKey", "priyankadhayapule-dhs_tablet-app-source-code")
        property("sonar.organization", "priyankadhayapule-dhs")
        property("sonar.projectKey", "priyankadhayapule-dhs_HydroGuideApp")
        property("sonar.organization", "priyankadhayapule-dhs")
        property("sonar.host.url", "https://sonarcloud.io")
        // Android Lint report path(s)
        property("sonar.androidLint.reportPaths", "thorsdk/build/reports/lint-results-basicDebug.xml, app/build/reports/lint-results-debug.xml")
    }
}