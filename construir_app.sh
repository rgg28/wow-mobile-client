#!/bin/bash
set -e

echo "🚀 Creando estructura del proyecto Android..."

# 1. Crear el árbol de directorios obligatorio
mkdir -p app/src/main/java/com/wowmobile/gfnow
mkdir -p app/src/main/res/values

# 2. Configuración global de Gradle
cat << 'EOF' > settings.gradle.kts
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}
rootProject.name = "WoWGFNMobile"
include(":app")
EOF

# 3. Build Gradle Raíz
cat << 'EOF' > build.gradle.kts
plugins {
    id("com.android.application") version "8.1.0" apply false
    id("org.jetbrains.kotlin.android") version "1.8.20" apply false
}
EOF

# 4. Build Gradle del Módulo App
cat << 'EOF' > app/build.gradle.kts
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.wowmobile.gfnow"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.wowmobile.gfnow"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }
    
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.11.0")
}
EOF

# 5. Manifiesto de Android con permisos de Internet y orientación forzada
cat << 'EOF' > app/src/main/AndroidManifest.xml
<manifest xmlns:android="http://android.com"
    package="com.wowmobile.gfnow">
    
    <uses-permission android:name="android.permission.INTERNET" />

    <application
        android:hardwareAccelerated="true"
        android:allowBackup="true"
        android:icon="@android:drawable/sym_def_app_icon"
        android:label="WoW GFN Mobile"
        android:theme="@style/Theme.AppCompat.NoActionBar">
        
        <activity
            android:name=".MainActivity"
            android:exported="true"
            android:screenOrientation="landscape">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
EOF

# 6. Strings básicas
cat << 'EOF' > app/src/main/res/values/strings.xml
<resources>
    <string name="app_name">WoW GFN Mobile</string>
</resources>
EOF

# 7. MainActivity (Carga WebView + inyección de inputs)
cat << 'EOF' > app/src/main/java/com/wowmobile/gfnow/MainActivity.kt
package com.wowmobile.gfnow

import android.annotation.SuppressLint
import android.os.Bundle
import android.view.KeyEvent
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var webView: WebView

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val rootLayout = FrameLayout(this)
        setContentView(rootLayout)

        webView = WebView(this).apply {
            webViewClient = WebViewClient()
            settings.apply {
                javaScriptEnabled = true
                domStorageEnabled = true
                useWideViewPort = true
                loadWithOverviewMode = true
                // Simula Chromebook para evadir restricciones de navegador en ://geforcenow.com
                userAgentString = "Mozilla/5.0 (X11; CrOS x86_64 14541.0.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
            }
            loadUrl("https://://geforcenow.com")
        }
        rootLayout.addView(webView)

        // Capa superior con el arco de botones virtuales
        val floatingButtons = FloatingButtons(this) { botonId ->
            inyectarTeclaWoW(botonId)
        }
        rootLayout.addView(floatingButtons)
    }

    private fun inyectarTeclaWoW(botonId: Int) {
        val keyCode = when (botonId) {
            0 -> KeyEvent.KEYCODE_1  # Mapeado a la barra de acción principal de WoW
            1 -> KeyEvent.KEYCODE_2
            2 -> KeyEvent.KEYCODE_3
            3 -> KeyEvent.KEYCODE_4
            else -> KeyEvent.KEYCODE_UNKNOWN
        }

        if (keyCode != KeyEvent.KEYCODE_UNKNOWN) {
            webView.dispatchKeyEvent(KeyEvent(KeyEvent.ACTION_DOWN, keyCode))
            webView.dispatchKeyEvent(KeyEvent(KeyEvent.ACTION_UP, keyCode))
        }
    }
}
EOF

# 8. FloatingButtons (Geometría radial de los botones táctiles)
cat << 'EOF' > app/src/main/java/com/wowmobile/gfnow/FloatingButtons.kt
package com.wowmobile.gfnow

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.widget.Button
import android.widget.FrameLayout
import kotlin.math.cos
import kotlin.math.sin

class FloatingButtons(context: Context, private val onButtonClick: (Int) -> Unit) : FrameLayout(context) {

    init {
        val centerRadius = 130
        val skillRadius = 75
        val distance = 240

        // Botón de ataque principal (Centro)
        val mainBtn = Button(context).apply {
            text = "⚔️"
            setBackgroundColor(Color.parseColor("#80000000"))
            setTextColor(Color.WHITE)
            setOnClickListener { onButtonClick(0) }
        }
        val mainParams = LayoutParams(centerRadius * 2, centerRadius * 2).apply {
            gravity = Gravity.BOTTOM or Gravity.END
            setMargins(0, 0, 80, 80)
        }
        addView(mainBtn, mainParams)

        // Habilidades secundarias en arco radial
        val totalSkills = 3
        val startAngle = 110.0
        val endAngle = 180.0
        val angleStep = (endAngle - startAngle) / (totalSkills - 1)

        for (i in 0 until totalSkills) {
            val angleRad = Math.toRadians(startAngle + (i * angleStep))

            val skillBtn = Button(context).apply {
                text = "${i + 2}"
                setBackgroundColor(Color.parseColor("#A0222222"))
                setTextColor(Color.WHITE)
                setOnClickListener { onButtonClick(i + 1) }
            }

            val tx = -(distance * cos(angleRad)).toFloat()
            val ty = -(distance * sin(angleRad)).toFloat()

            val skillParams = LayoutParams(skillRadius * 2, skillRadius * 2).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                setMargins(0, 0, 80 + centerRadius - skillRadius, 80 + centerRadius - skillRadius)
            }
            skillBtn.translationX = tx
            skillBtn.translationY = ty

            addView(skillBtn, skillParams)
        }
    }
}
EOF

echo "📥 Inicializando entorno Gradle..."
# Instala de manera silenciosa los archivos necesarios para ejecutar Gradle sin Android Studio
dotnet_installed=$(command -v gradle &> /dev/null && echo "yes" || echo "no")
if [ "$dotnet_installed" = "yes" ]; then
    gradle wrapper --gradle-version 8.1.1
else
    # Si no tienes gradle global, descarga el wrapper básico
    echo "⚠️ Gradle no encontrado globalmente. Intentando descargar componentes mínimos de compilación..."
    curl -L https://gradle.org -o gradle.zip
    unzip -q gradle.zip
    ./gradle-8.1.1/bin/gradle wrapper
    rm -rf gradle.zip gradle-8.1.1
fi

chmod +x gradlew

echo "🏗️ Compilando el archivo APK... (Esto puede demorar unos minutos la primera vez)"
./gradlew assembleDebug

echo "🎉 ¡Listo! Tu aplicación compilada se encuentra en:"
echo "$(pwd)/app/build/outputs/apk/debug/app-debug.apk"
