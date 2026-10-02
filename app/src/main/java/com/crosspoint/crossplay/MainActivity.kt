package com.crosspoint.crossplay

import android.Manifest
import android.app.NativeActivity
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.File
import java.io.FileOutputStream

class MainActivity : NativeActivity() {

    companion object {
        private const val TAG = "CrossPlayActivity"
    }

    private external fun nativeSwitchApp(appId: Int)
    private external fun nativeOnDownloadProgress(downloaded: Long, total: Long): Boolean

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setFullScreenMode()
        checkStoragePermissions()
        handleIncomingFileIntent(intent)
    }

    fun setScreenBrightnessFromNative(percent: Int) {
        runOnUiThread {
            try {
                val lp = window.attributes
                lp.screenBrightness = if (percent <= 0) 0.01f else (percent.coerceIn(1, 100) / 100.0f)
                window.attributes = lp
            } catch (e: Exception) {
                Log.e(TAG, "Failed to set screen brightness", e)
            }
        }
    }

    fun getBatteryPercentage(): Int {
        return try {
            val bm = getSystemService(BATTERY_SERVICE) as? android.os.BatteryManager
            bm?.getIntProperty(android.os.BatteryManager.BATTERY_PROPERTY_CAPACITY) ?: 100
        } catch (e: Exception) {
            100
        }
    }

    fun isBatteryCharging(): Boolean {
        return try {
            val intent = registerReceiver(null, android.content.IntentFilter(Intent.ACTION_BATTERY_CHANGED))
            val status = intent?.getIntExtra(android.os.BatteryManager.EXTRA_STATUS, -1) ?: -1
            status == android.os.BatteryManager.BATTERY_STATUS_CHARGING ||
                   status == android.os.BatteryManager.BATTERY_STATUS_FULL
        } catch (e: Exception) {
            false
        }
    }

    private fun setFullScreenMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.insetsController?.let { controller ->
                controller.hide(WindowInsets.Type.statusBars() or WindowInsets.Type.navigationBars())
                controller.systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        } else {
            @Suppress("DEPRECATION")
            window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            )
        }
    }

    private fun checkStoragePermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                try {
                    val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                        data = Uri.parse("package:$packageName")
                    }
                    startActivityForResult(intent, 1002)
                } catch (e: Exception) {
                    val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                    startActivityForResult(intent, 1002)
                }
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(
                        Manifest.permission.READ_EXTERNAL_STORAGE,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE
                    ),
                    1001
                )
            }
        }
    }

    private fun handleIncomingFileIntent(intent: Intent?) {
        if (intent == null || intent.action != Intent.ACTION_VIEW) return
        val uri = intent.data ?: return

        try {
            var fileName: String? = null
            if (uri.scheme == "content") {
                contentResolver.query(uri, arrayOf(android.provider.OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
                    if (cursor.moveToFirst()) {
                        val nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
                        if (nameIndex != -1) {
                            fileName = cursor.getString(nameIndex)
                        }
                    }
                }
            }
            if (fileName.isNullOrEmpty()) {
                fileName = uri.lastPathSegment?.substringAfterLast('/') ?: "imported_book.epub"
            }
            val safeFileName = File(fileName!!).name

            val targetDir = File(getExternalFilesDir(null), "CrossPlay/Books").apply { mkdirs() }
            val targetFile = File(targetDir, safeFileName)

            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(targetFile).use { output ->
                    input.copyTo(output)
                }
            }
            Toast.makeText(this, "Imported book: ${targetFile.name}", Toast.LENGTH_LONG).show()
            nativeSwitchApp(1) // Open EPUB Reader App
        } catch (e: Exception) {
            Log.e(TAG, "Failed to import file from intent", e)
        }
    }

    // JNI Network Helpers called from C++ Engine
    fun fetchUrlFromJava(urlString: String): String {
        var currentUrl = urlString
        var redirectCount = 0
        val maxRedirects = 5

        while (redirectCount < maxRedirects) {
            try {
                val url = java.net.URL(currentUrl)
                val conn = url.openConnection() as java.net.HttpURLConnection
                conn.connectTimeout = 10000
                conn.readTimeout = 10000
                conn.requestMethod = "GET"
                conn.instanceFollowRedirects = true
                conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Android; Mobile) CrossPlay/1.0")

                val status = conn.responseCode
                if (status in 200..299) {
                    return conn.inputStream.bufferedReader().use { it.readText() }
                } else if (status in 300..399) {
                    val newUrl = conn.getHeaderField("Location")
                    if (newUrl.isNullOrEmpty()) {
                        Log.e(TAG, "Redirect $status without Location header for $currentUrl")
                        return ""
                    }
                    currentUrl = if (newUrl.startsWith("http://") || newUrl.startsWith("https://")) {
                        newUrl
                    } else {
                        java.net.URL(url, newUrl).toString()
                    }
                    redirectCount++
                } else {
                    Log.e(TAG, "HTTP fetch failed with code $status for $currentUrl")
                    return ""
                }
            } catch (e: Exception) {
                Log.e(TAG, "HTTPS Fetch error: $currentUrl", e)
                return ""
            }
        }
        return ""
    }

    fun downloadFileFromJava(urlString: String, savePath: String): Boolean {
        var currentUrl = urlString
        var redirectCount = 0
        val maxRedirects = 5

        while (redirectCount < maxRedirects) {
            try {
                val url = java.net.URL(currentUrl)
                val conn = url.openConnection() as java.net.HttpURLConnection
                conn.connectTimeout = 15000
                conn.readTimeout = 15000
                conn.instanceFollowRedirects = true
                conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Android; Mobile) CrossPlay/1.0")

                val status = conn.responseCode
                if (status in 200..299) {
                    val totalLength = conn.contentLengthLong
                    val file = File(savePath)
                    file.parentFile?.mkdirs()
                    var downloaded = 0L
                    var lastProgressTime = 0L

                    var cont = true
                    conn.inputStream.use { input ->
                        FileOutputStream(file).use { output ->
                            val buffer = ByteArray(32768)
                            var read: Int
                            while (input.read(buffer).also { read = it } != -1) {
                                output.write(buffer, 0, read)
                                downloaded += read
                                val now = System.currentTimeMillis()
                                if (now - lastProgressTime >= 100) {
                                    lastProgressTime = now
                                    cont = nativeOnDownloadProgress(downloaded, totalLength)
                                    if (!cont) break
                                }
                            }
                        }
                    }
                    if (!cont) {
                        file.delete()
                        Log.i(TAG, "Download cancelled: $urlString")
                        return false
                    }
                    nativeOnDownloadProgress(downloaded, totalLength)
                    Log.i(TAG, "Downloaded $urlString -> $savePath (${file.length()} bytes)")
                    return true
                } else if (status in 300..399) {
                    val newUrl = conn.getHeaderField("Location")
                    if (newUrl.isNullOrEmpty()) return false
                    currentUrl = if (newUrl.startsWith("http://") || newUrl.startsWith("https://")) {
                        newUrl
                    } else {
                        java.net.URL(url, newUrl).toString()
                    }
                    redirectCount++
                } else {
                    Log.e(TAG, "HTTP download failed with code $status for $currentUrl")
                    return false
                }
            } catch (e: Exception) {
                Log.e(TAG, "Download error: $currentUrl -> $savePath", e)
                return false
            }
        }
        return false
    }
}
