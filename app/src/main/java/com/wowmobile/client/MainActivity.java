package com.wowmobile.client;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.TextView;
import android.widget.Toast;

import androidx.documentfile.provider.DocumentFile;

public class MainActivity extends Activity {

    private static final int REQUEST_WOW_FOLDER = 1001;

    private LinearLayout root;
    private LinearLayout content;

    private TextView title;
    private TextView status;

    private Uri wowFolderUri;

    private EditText usernameInput;
    private EditText passwordInput;

    private Button continueButton;
    private Button loginButton;

    private static final int BG = Color.rgb(12, 14, 18);
    private static final int PANEL = Color.rgb(24, 27, 34);
    private static final int PANEL2 = Color.rgb(31, 35, 44);
    private static final int TEXT = Color.rgb(235, 235, 235);
    private static final int SUBTEXT = Color.rgb(160, 165, 175);
    private static final int ACCENT = Color.rgb(190, 150, 60);

    static {
        System.loadLibrary("wowmobile");
    }

    private native void nativeInit();
    private native void nativeSetWowFolder(String path);
    private native void nativeLogin(String username, String password);
    private native void nativeSelectCharacter(int index);
    private native void nativeTouch(int action, float x, float y);
    private native void nativeJoystick(float x, float y);
    private native void nativeSpell(int spellId);
    private native void nativeJump();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeInit();

        crearInterfazBase();

        Uri saved = obtenerCarpetaGuardada();

        if (saved != null) {
            wowFolderUri = saved;

            if (validarCliente(saved)) {
                mostrarClienteEncontrado(saved);
            } else {
                mostrarSeleccionCliente();
            }
        } else {
            mostrarSeleccionCliente();
        }
    }

    private void crearInterfazBase() {

        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setBackgroundColor(BG);

        setContentView(root);
    }

    private void limpiar() {
        root.removeAllViews();
    }

    private TextView texto(String text, float size) {

        TextView v = new TextView(this);

        v.setText(text);
        v.setTextColor(TEXT);
        v.setTextSize(size);
        v.setGravity(Gravity.CENTER);

        return v;
    }

    private TextView textoSecundario(String text) {

        TextView v = new TextView(this);

        v.setText(text);
        v.setTextColor(SUBTEXT);
        v.setTextSize(15);
        v.setGravity(Gravity.CENTER);

        v.setPadding(20, 8, 20, 8);

        return v;
    }

    private Button boton(String text) {

        Button b = new Button(this);

        b.setText(text);
        b.setTextColor(TEXT);
        b.setTextSize(15);
        b.setAllCaps(false);

        b.setBackgroundColor(PANEL2);

        return b;
    }

    private Space espacio(int dp) {

        Space s = new Space(this);

        s.setLayoutParams(
                new LinearLayout.LayoutParams(
                        1,
                        dp
                )
        );

        return s;
    }

    private LinearLayout panel() {

        LinearLayout p = new LinearLayout(this);

        p.setOrientation(LinearLayout.VERTICAL);
        p.setGravity(Gravity.CENTER);
        p.setPadding(35, 30, 35, 30);
        p.setBackgroundColor(PANEL);

        return p;
    }

    // ============================================================
    // SELECCION DEL CLIENTE
    // ============================================================

    private void mostrarSeleccionCliente() {

        limpiar();

        LinearLayout p = panel();

        TextView t = texto("WoW Mobile Client", 28);
        t.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        p.addView(t);

        p.addView(espacio(12));

        p.addView(textoSecundario(
                "Selecciona la carpeta donde se encuentran los archivos extraídos del cliente WoW 3.3.5a."
        ));

        p.addView(espacio(20));

        Button select = boton("Seleccionar carpeta del cliente");

        select.setOnClickListener(v -> abrirSelectorCarpeta());

        p.addView(select);

        p.addView(espacio(15));

        status = textoSecundario(
                "Ningún cliente seleccionado."
        );

        p.addView(status);

        root.addView(
                p,
                new LinearLayout.LayoutParams(
                        500,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    private void abrirSelectorCarpeta() {

        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION |
                Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION |
                Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );

        startActivityForResult(intent, REQUEST_WOW_FOLDER);
    }

    @Override
    protected void onActivityResult(
            int requestCode,
            int resultCode,
            Intent data
    ) {

        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode != REQUEST_WOW_FOLDER) {
            return;
        }

        if (resultCode != RESULT_OK || data == null) {
            mostrarSeleccionCliente();
            return;
        }

        Uri uri = data.getData();

        if (uri == null) {
            mostrarSeleccionCliente();
            return;
        }

        try {

            int flags =
                    data.getFlags() &
                    (Intent.FLAG_GRANT_READ_URI_PERMISSION |
                     Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

            getContentResolver().takePersistableUriPermission(
                    uri,
                    flags & Intent.FLAG_GRANT_READ_URI_PERMISSION
            );

        } catch (Exception ignored) {
        }

        wowFolderUri = uri;

        guardarCarpeta(uri);

        if (validarCliente(uri)) {
            mostrarClienteEncontrado(uri);
        } else {
            mostrarClienteNoValido(uri);
        }
    }

    // ============================================================
    // VALIDACION DEL CLIENTE
    // ============================================================

    private boolean validarCliente(Uri uri) {

        DocumentFile rootFolder =
                DocumentFile.fromTreeUri(this, uri);

        if (rootFolder == null || !rootFolder.isDirectory()) {
            return false;
        }

        boolean cameras = false;
        boolean character = false;
        boolean creatures = false;

        DocumentFile[] files = rootFolder.listFiles();

        for (DocumentFile file : files) {

            String name = file.getName();

            if (name == null) {
                continue;
            }

            if (name.equalsIgnoreCase("cameras") && file.isDirectory()) {
                cameras = true;
            }

            if (name.equalsIgnoreCase("character") && file.isDirectory()) {
                character = true;
            }

            if (name.equalsIgnoreCase("creatures") && file.isDirectory()) {
                creatures = true;
            }
        }

        return cameras || character || creatures;
    }

    private boolean existeCarpeta(Uri uri, String nombre) {

        DocumentFile rootFolder =
                DocumentFile.fromTreeUri(this, uri);

        if (rootFolder == null) {
            return false;
        }

        for (DocumentFile file : rootFolder.listFiles()) {

            String name = file.getName();

            if (name != null &&
                    name.equalsIgnoreCase(nombre) &&
                    file.isDirectory()) {

                return true;
            }
        }

        return false;
    }

    private void mostrarClienteNoValido(Uri uri) {

        limpiar();

        LinearLayout p = panel();

        TextView t = texto("Cliente no reconocido", 25);
        t.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        p.addView(t);

        p.addView(espacio(15));

        p.addView(textoSecundario(
                "La carpeta seleccionada no contiene una estructura reconocible del cliente."
        ));

        p.addView(espacio(15));

        p.addView(textoSecundario(
                "Selecciona la carpeta raíz que contiene carpetas como cameras, character, creatures, etc."
        ));

        p.addView(espacio(20));

        Button retry = boton("Seleccionar otra carpeta");

        retry.setOnClickListener(v -> abrirSelectorCarpeta());

        p.addView(retry);

        root.addView(
                p,
                new LinearLayout.LayoutParams(
                        550,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    // ============================================================
    // CLIENTE ENCONTRADO
    // ============================================================

    private void mostrarClienteEncontrado(Uri uri) {

        limpiar();

        LinearLayout p = panel();

        TextView t = texto("Cliente WoW encontrado", 26);
        t.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        p.addView(t);

        p.addView(espacio(15));

        p.addView(textoSecundario(
                "La carpeta seleccionada contiene archivos del cliente."
        ));

        p.addView(espacio(15));

        agregarResultado(
                p,
                "cameras/",
                existeCarpeta(uri, "cameras")
        );

        agregarResultado(
                p,
                "character/",
                existeCarpeta(uri, "character")
        );

        agregarResultado(
                p,
                "creatures/",
                existeCarpeta(uri, "creatures")
        );

        p.addView(espacio(20));

        continueButton = boton("CONTINUAR AL LOGIN");

        continueButton.setOnClickListener(
                v -> mostrarLogin()
        );

        p.addView(continueButton);

        p.addView(espacio(10));

        Button change = boton("Cambiar carpeta");

        change.setOnClickListener(
                v -> abrirSelectorCarpeta()
        );

        p.addView(change);

        root.addView(
                p,
                new LinearLayout.LayoutParams(
                        550,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );

        try {
            nativeSetWowFolder(uri.toString());
        } catch (Exception ignored) {
        }
    }

    private void agregarResultado(
            LinearLayout parent,
            String nombre,
            boolean encontrado
    ) {

        TextView v = textoSecundario(
                (encontrado ? "✓ " : "○ ") + nombre
        );

        v.setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);

        parent.addView(v);
    }

    // ============================================================
    // LOGIN
    // ============================================================

    private void mostrarLogin() {

        limpiar();

        LinearLayout p = panel();

        TextView t = texto("Iniciar sesión", 28);
        t.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        p.addView(t);

        p.addView(espacio(8));

        p.addView(textoSecundario(
                "WoW 3.3.5a"
        ));

        p.addView(espacio(20));

        TextView userLabel = textoSecundario("Usuario");

        userLabel.setGravity(Gravity.LEFT);

        p.addView(userLabel);

        usernameInput = new EditText(this);

        usernameInput.setSingleLine(true);
        usernameInput.setTextColor(TEXT);
        usernameInput.setHintTextColor(SUBTEXT);
        usernameInput.setHint("Nombre de usuario");
        usernameInput.setTextSize(17);
        usernameInput.setPadding(18, 10, 18, 10);

        p.addView(
                usernameInput,
                new LinearLayout.LayoutParams(
                        450,
                        60
                )
        );

        p.addView(espacio(10));

        TextView passLabel = textoSecundario("Contraseña");

        passLabel.setGravity(Gravity.LEFT);

        p.addView(passLabel);

        passwordInput = new EditText(this);

        passwordInput.setSingleLine(true);
        passwordInput.setTextColor(TEXT);
        passwordInput.setHintTextColor(SUBTEXT);
        passwordInput.setHint("Contraseña");
        passwordInput.setTextSize(17);
        passwordInput.setInputType(
                android.text.InputType.TYPE_CLASS_TEXT |
                android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
        );

        passwordInput.setPadding(18, 10, 18, 10);

        p.addView(
                passwordInput,
                new LinearLayout.LayoutParams(
                        450,
                        60
                )
        );

        p.addView(espacio(20));

        loginButton = boton("CONECTAR");

        loginButton.setOnClickListener(
                v -> intentarLogin()
        );

        p.addView(
                loginButton,
                new LinearLayout.LayoutParams(
                        300,
                        60
                )
        );

        p.addView(espacio(12));

        status = textoSecundario(
                "Estado: cliente cargado. Sin conexión todavía."
        );

        p.addView(status);

        p.addView(espacio(10));

        Button back = boton("Volver");

        back.setOnClickListener(
                v -> mostrarClienteEncontrado(wowFolderUri)
        );

        p.addView(back);

        root.addView(
                p,
                new LinearLayout.LayoutParams(
                        600,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    private void intentarLogin() {

        String username =
                usernameInput.getText().toString().trim();

        String password =
                passwordInput.getText().toString();

        if (username.isEmpty()) {

            usernameInput.requestFocus();

            Toast.makeText(
                    this,
                    "Introduce el usuario.",
                    Toast.LENGTH_SHORT
            ).show();

            return;
        }

        if (password.isEmpty()) {

            passwordInput.requestFocus();

            Toast.makeText(
                    this,
                    "Introduce la contraseña.",
                    Toast.LENGTH_SHORT
            ).show();

            return;
        }

        /*
         * IMPORTANTE:
         *
         * Todavía NO existe la implementación real del protocolo
         * de autenticación WoW 3.3.5a.
         *
         * Por eso no cambiamos de pantalla ni fingimos conexión.
         */

        status.setText(
                "Estado: autenticación WoW todavía no implementada."
        );

        try {
            nativeLogin(username, password);
        } catch (Exception ignored) {
        }
    }

    // ============================================================
    // CARPETA GUARDADA
    // ============================================================

    private void guardarCarpeta(Uri uri) {

        SharedPreferences prefs =
                getSharedPreferences(
                        "wow_client",
                        MODE_PRIVATE
                );

        prefs.edit()
                .putString(
                        "wow_folder_uri",
                        uri.toString()
                )
                .apply();
    }

    private Uri obtenerCarpetaGuardada() {

        SharedPreferences prefs =
                getSharedPreferences(
                        "wow_client",
                        MODE_PRIVATE
                );

        String value =
                prefs.getString(
                        "wow_folder_uri",
                        null
                );

        if (value == null || value.isEmpty()) {
            return null;
        }

        try {
            return Uri.parse(value);
        } catch (Exception e) {
            return null;
        }
    }
}
