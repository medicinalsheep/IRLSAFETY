plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

val irlsafetyRepoRoot = file("../../").absolutePath
val detectionModelSrc = file("$irlsafetyRepoRoot/data/models/irlsafety-detect.onnx")

android {
    namespace = "com.irlsafety.plus"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.irlsafety.plus"
        minSdk = 26
        targetSdk = 35
        versionCode = 905
        versionName = "0.9.5-dev"

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++20", "-frtti", "-fexceptions")
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DIRLSAFETY_ENABLE_ONNX=ON",
                    "-DIRLSAFETY_OCR_BACKEND=stub"
                )
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        compose = true
        prefab = true
        buildConfig = true
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2024.10.01")
    val cameraxVersion = "1.4.0"
    val ortVersion = "1.20.1"

    implementation(composeBom)
    implementation("androidx.activity:activity-compose:1.9.3")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.core:core-ktx:1.15.0")
    implementation("androidx.camera:camera-core:$cameraxVersion")
    implementation("androidx.camera:camera-camera2:$cameraxVersion")
    implementation("androidx.camera:camera-lifecycle:$cameraxVersion")
    implementation("androidx.camera:camera-view:$cameraxVersion")
    implementation("com.microsoft.onnxruntime:onnxruntime-android:$ortVersion")
    debugImplementation("androidx.compose.ui:ui-tooling")
}

val copyDetectionModel = tasks.register<Copy>("copyDetectionModel") {
    description = "Stage irlsafety-detect.onnx into APK assets (P12)."
    from(detectionModelSrc)
    into(layout.projectDirectory.dir("src/main/assets/models"))
    onlyIf { detectionModelSrc.exists() }
}

tasks.named("preBuild") {
    dependsOn(copyDetectionModel)
}