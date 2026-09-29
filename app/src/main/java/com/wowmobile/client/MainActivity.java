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
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.documentfile.provider.DocumentFile;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends Activity {

    private static final int REQUEST_WOW_FOLDER = 1001;

    private static final String PREFS_NAME = "wow_mobile_client";
    private static final String PREF_WOW_URI = "wow_folder_uri";

    private static final int BG = Color.rgb(8, 14, 28);
    private static final int PANEL = Color.rgb(15, 25, 45);
    private static final int PANEL_2 = Color.rgb(20, 33, 58);
    private static final int GOLD = Color.rgb(222, 176, 72);
    private static final int GOLD_LIGHT = Color.rgb(245, 210, 120);
    private static final int WHITE = Color.rgb(235, 240, 248);
    private static final int MUTED = Color.rgb(155, 170, 195);
    private static final int GREEN = Color.rgb(75, 205, 120);
    private static final int RED = Color.rgb(225, 80, 80);
    private static final int BLUE = Color.rgb(75, 155, 230);

    private Uri wowFolderUri;

    private LinearLayout rootLayout;
    private TextView titleText;
    private TextView statusText;

    private EditText usernameInput;
    private EditText passwordInput;

    private Button testVfsButton;
    private Button continueLoginButton;

    static {
        System.loadLibrary("wowmobile");
    }

    private native void nativeInit();
    private native void nativeSetWowFolder(String uri);
    private native String nativeTestVfs();
    private native void nativeLogin(String username, String password);

    private native void nativeTouchDown(float x, float y);
    private native void nativeTouchMove(float x, float y);
    private native void nativeTouchUp(float x, float y);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);

        nativeInit();

        crearPantallaInicial();

        cargarCarpetaGuardada();
    }

    // ============================================================
    // UI BASE
    // ============================================================

    private void prepararRoot() {
        rootLayout = new LinearLayout(this);
        rootLayout.setOrientation(LinearLayout.VERTICAL);
        rootLayout.setGravity(Gravity.CENTER_HORIZONTAL);
        rootLayout.setPadding(dp(28), dp(24), dp(28), dp(24));
        rootLayout.setBackgroundColor(BG);

        setContentView(rootLayout);
    }

    private TextView crearTitulo(String texto, int size) {
        TextView view = new TextView(this);

        view.setText(texto);
        view.setTextColor(GOLD_LIGHT);
        view.setTextSize(size);
        view.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD));
        view.setGravity(Gravity.CENTER);
        view.setLetterSpacing(0.05f);

        return view;
    }

    private TextView crearTexto(String texto, int size, int color) {
        TextView view = new TextView(this);

        view.setText(texto);
        view.setTextColor(color);
        view.setTextSize(size);
        view.setGravity(Gravity.CENTER);

        return view;
    }

    private Button crearBoton(String texto) {
        Button button = new Button(this);

        button.setText(texto);
        button.setTextColor(WHITE);
        button.setTextSize(14);
        button.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        button.setAllCaps(false);
        button.setMinHeight(dp(52));
        button.setPadding(dp(16), dp(8), dp(16), dp(8));

        return button;
    }

    private View separador(int height) {
        View view = new View(this);
        view.setLayoutParams(
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(height)
                )
        );
        return view;
    }

    private void agregar(View view) {
        rootLayout.addView(view);
    }

    private void agregar(View view, int width, int height) {
        LinearLayout.LayoutParams params =
                new LinearLayout.LayoutParams(
                        width == -1 ? ViewGroup.LayoutParams.MATCH_PARENT : dp(width),
                        height == -1 ? ViewGroup.LayoutParams.WRAP_CONTENT : dp(height)
                );

        rootLayout.addView(view, params);
    }

    // ============================================================
    // PANTALLA INICIAL
    // ============================================================

    private void crearPantallaInicial() {
        prepararRoot();

        agregar(separador(10));

        titleText = crearTitulo("WORLD OF WARCRAFT", 26);
        agregar(titleText, -1, -2);

        TextView subtitle =
                crearTexto("MOBILE CLIENT", 16, GOLD);
        subtitle.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        agregar(subtitle, -1, -2);

        agregar(separador(24));

        TextView description = crearTexto(
                "Selecciona la carpeta donde tienes los archivos\n" +
                "extraídos del cliente de World of Warcraft 3.3.5a.",
                15,
                MUTED
        );

        agregar(description, -1, -2);

        agregar(separador(24));

        Button selectButton = crearBoton("SELECCIONAR CARPETA DEL CLIENTE");
        selectButton.setOnClickListener(v -> seleccionarCarpeta());

        agregar(selectButton, -1, -2);

        agregar(separador(20));

        statusText = crearTexto(
                "Esperando selección del cliente...",
                14,
                MUTED
        );

        agregar(statusText, -1, -2);

        agregar(separador(20));

        TextView info = crearTexto(
                "El cliente permanece fuera del APK.\n" +
                "La aplicación accede a los archivos mediante almacenamiento SAF.",
                12,
                MUTED
        );

        agregar(info, -1, -2);
    }

    // ============================================================
    // CARPETA DEL CLIENTE
    // ============================================================

    private void seleccionarCarpeta() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION |
                Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION |
                Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );

        startActivityForResult(intent, REQUEST_WOW_FOLDER);
    }

    private void cargarCarpetaGuardada() {
        SharedPreferences prefs =
                getSharedPreferences(PREFS_NAME, MODE_PRIVATE);

        String savedUri = prefs.getString(PREF_WOW_URI, null);

        if (savedUri == null || savedUri.isEmpty()) {
            return;
        }

        try {
            Uri uri = Uri.parse(savedUri);

            if (uri == null) {
                return;
            }

            int flags =
                    Intent.FLAG_GRANT_READ_URI_PERMISSION |
                    Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION;

            try {
                getContentResolver().takePersistableUriPermission(uri, flags);
            } catch (Exception ignored) {
            }

            wowFolderUri = uri;

            nativeSetWowFolder(uri.toString());

            if (validarCliente()) {
                mostrarClienteEncontrado();
            }

        } catch (Exception e) {
            statusText.setText(
                    "No se pudo recuperar la carpeta guardada."
            );
            statusText.setTextColor(RED);
        }
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
            return;
        }

        Uri uri = data.getData();

        if (uri == null) {
            return;
        }

        try {
            int flags =
                    data.getFlags() &
                    (Intent.FLAG_GRANT_READ_URI_PERMISSION |
                     Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

            getContentResolver().takePersistableUriPermission(uri, flags);
        } catch (Exception ignored) {
        }

        wowFolderUri = uri;

        getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                .edit()
                .putString(PREF_WOW_URI, uri.toString())
                .apply();

        nativeSetWowFolder(uri.toString());

        if (validarCliente()) {
            mostrarClienteEncontrado();
        } else {
            mostrarClienteNoValido();
        }
    }

    // ============================================================
    // VALIDACIÓN BÁSICA
    // ============================================================

    private boolean validarCliente() {
        if (wowFolderUri == null) {
            return false;
        }

        boolean cameras = vfsIsDirectory("cameras");
        boolean character = vfsIsDirectory("character");
        boolean creatures = vfsIsDirectory("creatures");

        return cameras || character || creatures;
    }

    private void mostrarClienteNoValido() {
        prepararRoot();

        agregar(separador(20));

        TextView title =
                crearTitulo("CLIENTE NO DETECTADO", 24);
        title.setTextColor(RED);
        agregar(title, -1, -2);

        agregar(separador(20));

        TextView message = crearTexto(
                "La carpeta seleccionada no parece contener\n" +
                "la estructura esperada del cliente.",
                15,
                WHITE
        );

        agregar(message, -1, -2);

        agregar(separador(25));

        TextView expected = crearTexto(
                "Se buscan carpetas como:\n\n" +
                "cameras\n" +
                "character\n" +
                "creatures",
                14,
                MUTED
        );

        agregar(expected, -1, -2);

        agregar(separador(25));

        Button retry = crearBoton("CAMBIAR CARPETA");
        retry.setOnClickListener(v -> seleccionarCarpeta());

        agregar(retry, -1, -2);
    }

    // ============================================================
    // CLIENTE DETECTADO
    // ============================================================

    private void mostrarClienteEncontrado() {
        prepararRoot();

        agregar(separador(8));

        TextView title =
                crearTitulo("CLIENTE DETECTADO", 25);
        agregar(title, -1, -2);

        agregar(separador(8));

        TextView subtitle =
                crearTexto(
                        "WoW 3.3.5a / VFS Android",
                        14,
                        MUTED
                );

        agregar(subtitle, -1, -2);

        agregar(separador(22));

        LinearLayout checks = new LinearLayout(this);
        checks.setOrientation(LinearLayout.VERTICAL);
        checks.setPadding(dp(20), dp(15), dp(20), dp(15));
        checks.setBackgroundColor(PANEL);

        agregarCheck(checks, "✓  cameras", vfsIsDirectory("cameras"));
        agregarCheck(checks, "✓  character", vfsIsDirectory("character"));
        agregarCheck(checks, "✓  creatures", vfsIsDirectory("creatures"));
        agregarCheck(checks, "✓  SAF / almacenamiento", wowFolderUri != null);
        agregarCheck(checks, "✓  VFS Android preparado", true);

        agregar(checks, -1, -2);

        agregar(separador(22));

        testVfsButton = crearBoton("PROBAR LECTURA DEL CLIENTE");

        testVfsButton.setOnClickListener(
                v -> ejecutarPruebaVfs()
        );

        agregar(testVfsButton, -1, -2);

        agregar(separador(12));

        continueLoginButton =
                crearBoton("CONTINUAR AL LOGIN");

        continueLoginButton.setEnabled(false);
        continueLoginButton.setAlpha(0.45f);

        continueLoginButton.setOnClickListener(
                v -> mostrarLogin()
        );

        agregar(continueLoginButton, -1, -2);

        agregar(separador(12));

        Button change =
                crearBoton("Cambiar carpeta");

        change.setOnClickListener(
                v -> seleccionarCarpeta()
        );

        agregar(change, -1, -2);

        agregar(separador(15));

        TextView note = crearTexto(
                "La prueba de lectura no modifica ningún archivo.",
                12,
                MUTED
        );

        agregar(note, -1, -2);
    }

    private void agregarCheck(
            LinearLayout parent,
            String text,
            boolean ok
    ) {
        TextView view =
                crearTexto(
                        ok ? text : text.replace("✓", "✗"),
                        14,
                        ok ? GREEN : RED
                );

        view.setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);
        view.setPadding(dp(4), dp(7), dp(4), dp(7));

        parent.addView(
                view,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(38)
                )
        );
    }

    // ============================================================
    // PRUEBA VFS
    // ============================================================

    private void ejecutarPruebaVfs() {
        if (wowFolderUri == null) {
            Toast.makeText(
                    this,
                    "Primero selecciona la carpeta del cliente.",
                    Toast.LENGTH_LONG
            ).show();

            return;
        }

        testVfsButton.setEnabled(false);
        testVfsButton.setText("LEYENDO CLIENTE...");

        String resultado;

        try {
            resultado = nativeTestVfs();
        } catch (Exception e) {
            resultado =
                    "VFS_ERROR\n" +
                    "Excepción durante la prueba: " +
                    e.getClass().getSimpleName() +
                    "\n" +
                    e.getMessage();
        }

        mostrarResultadoVfs(resultado);
    }

    private void mostrarResultadoVfs(String resultado) {
        prepararRoot();

        agregar(separador(8));

        TextView title =
                crearTitulo("PRUEBA DEL VFS", 24);
        agregar(title, -1, -2);

        agregar(separador(8));

        ScrollView scroll = new ScrollView(this);

        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(
                dp(18),
                dp(14),
                dp(18),
                dp(14)
        );
        content.setBackgroundColor(PANEL);

        if (resultado == null || resultado.trim().isEmpty()) {
            resultado =
                    "VFS_ERROR\n" +
                    "El motor nativo no devolvió ningún resultado.";
        }

        String[] lines = resultado.split("\\n");

        boolean success = false;

        for (String line : lines) {
            TextView row =
                    crearTexto(line, 13, WHITE);

            row.setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);
            row.setPadding(0, dp(4), 0, dp(4));

            if (line.startsWith("VFS_OK")) {
                row.setTextColor(GREEN);
                row.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
                success = true;
            } else if (line.startsWith("VFS_ERROR")) {
                row.setTextColor(RED);
                row.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
            } else if (line.startsWith("[OK]")) {
                row.setTextColor(GREEN);
            } else if (line.startsWith("[ERROR]")) {
                row.setTextColor(RED);
            } else if (line.startsWith("[INFO]")) {
                row.setTextColor(BLUE);
            } else {
                row.setTextColor(WHITE);
            }

            content.addView(row);
        }

        scroll.addView(content);

        LinearLayout.LayoutParams scrollParams =
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        0,
                        1.0f
                );

        rootLayout.addView(scroll, scrollParams);

        agregar(separador(12));

        continueLoginButton =
                crearBoton("CONTINUAR AL LOGIN");

        continueLoginButton.setEnabled(success);
        continueLoginButton.setAlpha(success ? 1.0f : 0.45f);

        continueLoginButton.setOnClickListener(
                v -> mostrarLogin()
        );

        agregar(continueLoginButton, -1, -2);

        agregar(separador(8));

        Button back =
                crearBoton("VOLVER");

        back.setOnClickListener(
                v -> mostrarClienteEncontrado()
        );

        agregar(back, -1, -2);
    }

    // ============================================================
    // LOGIN
    // ============================================================

    private void mostrarLogin() {
        prepararRoot();

        agregar(separador(10));

        TextView title =
                crearTitulo("WORLD OF WARCRAFT", 25);
        agregar(title, -1, -2);

        TextView subtitle =
                crearTexto(
                        "INICIAR SESIÓN",
                        15,
                        GOLD
                );

        subtitle.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        agregar(subtitle, -1, -2);

        agregar(separador(28));

        usernameInput =
                new EditText(this);

        usernameInput.setHint("Usuario");
        usernameInput.setHintTextColor(MUTED);
        usernameInput.setTextColor(WHITE);
        usernameInput.setTextSize(16);
        usernameInput.setSingleLine(true);
        usernameInput.setPadding(
                dp(16),
                0,
                dp(16),
                0
        );
        usernameInput.setBackgroundColor(PANEL_2);

        agregar(usernameInput, -1, 56);

        agregar(separador(14));

        passwordInput =
                new EditText(this);

        passwordInput.setHint("Contraseña");
        passwordInput.setHintTextColor(MUTED);
        passwordInput.setTextColor(WHITE);
        passwordInput.setTextSize(16);
        passwordInput.setSingleLine(true);
        passwordInput.setInputType(
                android.text.InputType.TYPE_CLASS_TEXT |
                android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
        );
        passwordInput.setPadding(
                dp(16),
                0,
                dp(16),
                0
        );
        passwordInput.setBackgroundColor(PANEL_2);

        agregar(passwordInput, -1, 56);

        agregar(separador(22));

        Button loginButton =
                crearBoton("CONECTAR");

        loginButton.setOnClickListener(
                v -> realizarLogin()
        );

        agregar(loginButton, -1, -2);

        agregar(separador(14));

        statusText =
                crearTexto(
                        "Protocolo WoW 3.3.5a pendiente de implementación.",
                        13,
                        BLUE
                );

        agregar(statusText, -1, -2);

        agregar(separador(18));

        Button back =
                crearBoton("VOLVER");

        back.setOnClickListener(
                v -> mostrarClienteEncontrado()
        );

        agregar(back, -1, -2);
    }

    private void realizarLogin() {
        String username =
                usernameInput.getText()
                        .toString()
                        .trim();

        String password =
                passwordInput.getText()
                        .toString();

        if (username.isEmpty()) {
            usernameInput.setError("Introduce el usuario.");
            usernameInput.requestFocus();
            return;
        }

        if (password.isEmpty()) {
            passwordInput.setError("Introduce la contraseña.");
            passwordInput.requestFocus();
            return;
        }

        statusText.setText(
                "Enviando solicitud de autenticación..."
        );
        statusText.setTextColor(BLUE);

        nativeLogin(username, password);
    }

    // ============================================================
    // VFS SAF
    // ============================================================

    private DocumentFile obtenerRaizVfs() {
        if (wowFolderUri == null) {
            return null;
        }

        try {
            return DocumentFile.fromTreeUri(
                    this,
                    wowFolderUri
            );
        } catch (Exception e) {
            return null;
        }
    }

    private DocumentFile encontrarDocumento(
            String relativePath
    ) {
        DocumentFile current =
                obtenerRaizVfs();

        if (current == null) {
            return null;
        }

        if (relativePath == null ||
                relativePath.isEmpty()) {
            return current;
        }

        String normalized =
                relativePath
                        .replace('\\', '/');

        while (normalized.startsWith("/")) {
            normalized =
                    normalized.substring(1);
        }

        String[] parts =
                normalized.split("/");

        for (String part : parts) {
            if (part.isEmpty()) {
                continue;
            }

            DocumentFile next = null;

            for (DocumentFile child :
                    current.listFiles()) {

                if (part.equals(child.getName())) {
                    next = child;
                    break;
                }
            }

            if (next == null) {
                return null;
            }

            current = next;
        }

        return current;
    }

    private boolean vfsExists(
            String relativePath
    ) {
        return encontrarDocumento(relativePath) != null;
    }

    private boolean vfsIsDirectory(
            String relativePath
    ) {
        DocumentFile file =
                encontrarDocumento(relativePath);

        return file != null && file.isDirectory();
    }

    private List<String> vfsList(
            String relativePath
    ) {
        List<String> result =
                new ArrayList<>();

        DocumentFile directory =
                encontrarDocumento(relativePath);

        if (directory == null ||
                !directory.isDirectory()) {
            return result;
        }

        for (DocumentFile file :
                directory.listFiles()) {

            String name = file.getName();

            if (name == null) {
                continue;
            }

            if (file.isDirectory()) {
                result.add(name + "/");
            } else {
                result.add(name);
            }
        }

        return result;
    }

    private byte[] vfsReadFile(
            String relativePath
    ) {
        DocumentFile file =
                encontrarDocumento(relativePath);

        if (file == null ||
                !file.isFile()) {
            return null;
        }

        try {
            long length = file.length();

            // Protección para no cargar archivos gigantes
            // completos en memoria durante la prueba.
            if (length > 16L * 1024L * 1024L) {
                return null;
            }

            try (InputStream input =
                         getContentResolver()
                                 .openInputStream(
                                         file.getUri()
                                 )) {

                if (input == null) {
                    return null;
                }

                ByteArrayOutputStream output =
                        new ByteArrayOutputStream();

                byte[] buffer =
                        new byte[8192];

                int read;

                while ((read =
                        input.read(buffer)) != -1) {

                    output.write(
                            buffer,
                            0,
                            read
                    );
                }

                return output.toByteArray();
            }

        } catch (Exception e) {
            return null;
        }
    }

    // ============================================================
    // TOUCH
    // ============================================================

    @Override
    public boolean dispatchTouchEvent(
            MotionEvent event
    ) {
        try {
            int action =
                    event.getActionMasked();

            float x = event.getX();
            float y = event.getY();

            if (action == MotionEvent.ACTION_DOWN) {
                nativeTouchDown(x, y);
            } else if (
                    action == MotionEvent.ACTION_MOVE
            ) {
                nativeTouchMove(x, y);
            } else if (
                    action == MotionEvent.ACTION_UP ||
                    action == MotionEvent.ACTION_CANCEL
            ) {
                nativeTouchUp(x, y);
            }

        } catch (Exception ignored) {
        }

        return super.dispatchTouchEvent(event);
    }

    // ============================================================
    // UTILIDADES
    // ============================================================

    private int dp(int value) {
        float density =
                getResources()
                        .getDisplayMetrics()
                        .density;

        return Math.round(value * density);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
