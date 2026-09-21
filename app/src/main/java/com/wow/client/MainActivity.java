package com.wow.client;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.widget.Toast;

public class MainActivity extends Activity {

    private static final int REQUEST_CODE_DATA_DIR = 1001;
    private static final String PREFS_NAME = "WoWAndroidPrefs";
    private static final String KEY_DATA_PATH = "wow_data_path";

    // Cargar la librería nativa de C++ compilada por CMake
    static {
        System.loadLibrary("wowclient");
    }

    // Declaración del método nativo en C++
    public native void iniciarMotorWoW(String rutaData);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String savedPath = getSavedDataPath();
        if (savedPath == null) {
            solicitarCarpetaData();
        } else {
            iniciarJuego(savedPath);
        }
    }

    private void solicitarCarpetaData() {
        Toast.makeText(this, "Por favor, selecciona tu carpeta 'Data' de WoW 3.3.5a", Toast.LENGTH_LONG).show();
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, Uri.parse("content://com.android.externalstorage.documents/document/primary%3A"));
        startActivityForResult(intent, REQUEST_CODE_DATA_DIR);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        
        if (requestCode == REQUEST_CODE_DATA_DIR && resultCode == Activity.RESULT_OK && data != null) {
            Uri treeUri = data.getData();
            
            // Hacer persistente el permiso de la carpeta
            final int takeFlags = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            getContentResolver().takePersistableUriPermission(treeUri, takeFlags);

            String pathString = treeUri.toString();
            SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit();
            editor.putString(KEY_DATA_PATH, pathString);
            editor.apply();

            iniciarJuego(pathString);
        } else {
            Toast.makeText(this, "Se requiere acceso a la carpeta Data para jugar.", Toast.LENGTH_LONG).show();
            finish();
        }
    }

    private String getSavedDataPath() {
        return getSharedPreferences(PREFS_NAME, MODE_PRIVATE).getString(KEY_DATA_PATH, null);
    }

    private void iniciarJuego(String dataPath) {
        Toast.makeText(this, "Iniciando cliente WoW...", Toast.LENGTH_SHORT).show();
        // Llamar al código nativo en C++ pasándole la ruta seleccionada
        iniciarMotorWoW(dataPath);
    }
}
