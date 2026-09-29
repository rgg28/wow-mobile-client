package com.wowmobile.client;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.text.InputType;
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
import java.util.Locale;

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

    private static final int MAX_SCAN_DEPTH = 8;
    private static final int MAX_SCAN_ITEMS = 50000;

    private Uri wowFolderUri;

    private LinearLayout rootLayout;
    private TextView statusText;

    private EditText usernameInput;
    private EditText passwordInput;

    private Button scanButton;
    private Button continueLoginButton;

    static {
        System.loadLibrary("wowmobile");
    }

    private native void nativeInit();

    private native void nativeSetWowFolder(String uri);

    private native String nativeScanClient();

    private native void nativeLogin(
            String username,
            String password
    );

    private native void nativeTouchDown(
            float x,
            float y
    );

    private native void nativeTouchMove(
            float x,
            float y
    );

    private native void nativeTouchUp(
            float x,
            float y
    );

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);

        nativeInit();

        mostrarSeleccionCliente();

        cargarCarpetaGuardada();
    }

    // ============================================================
    // UI
    // ============================================================

    private void prepararRoot() {
        rootLayout = new LinearLayout(this);

        rootLayout.setOrientation(
                LinearLayout.VERTICAL
        );

        rootLayout.setGravity(
                Gravity.CENTER_HORIZONTAL
        );

        rootLayout.setPadding(
                dp(28),
                dp(24),
                dp(28),
                dp(24)
        );

        rootLayout.setBackgroundColor(BG);

        setContentView(rootLayout);
    }

    private TextView titulo(
            String texto,
            int size
    ) {
        TextView view =
                new TextView(this);

        view.setText(texto);
        view.setTextColor(GOLD_LIGHT);
        view.setTextSize(size);
        view.setGravity(Gravity.CENTER);

        view.setTypeface(
                Typeface.create(
                        Typeface.SERIF,
                        Typeface.BOLD
                )
        );

        view.setLetterSpacing(0.05f);

        return view;
    }

    private TextView texto(
            String texto,
            int size,
            int color
    ) {
        TextView view =
                new TextView(this);

        view.setText(texto);
        view.setTextColor(color);
        view.setTextSize(size);

        view.setGravity(Gravity.CENTER);

        return view;
    }

    private Button boton(
            String texto
    ) {
        Button button =
                new Button(this);

        button.setText(texto);
        button.setTextColor(WHITE);
        button.setTextSize(14);

        button.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        button.setAllCaps(false);

        button.setMinHeight(
                dp(52)
        );

        button.setPadding(
                dp(16),
                dp(8),
                dp(16),
                dp(8)
        );

        return button;
    }

    private void agregar(View view) {
        rootLayout.addView(view);
    }

    private void agregar(
            View view,
            int width,
            int height
    ) {
        LinearLayout.LayoutParams params =
                new LinearLayout.LayoutParams(
                        width == -1
                                ? ViewGroup.LayoutParams.MATCH_PARENT
                                : dp(width),

                        height == -1
                                ? ViewGroup.LayoutParams.WRAP_CONTENT
                                : dp(height)
                );

        rootLayout.addView(
                view,
                params
        );
    }

    private View espacio(
            int height
    ) {
        View view =
                new View(this);

        view.setLayoutParams(
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(height)
                )
        );

        return view;
    }

    // ============================================================
    // PANTALLA SELECCIÓN
    // ============================================================

    private void mostrarSeleccionCliente() {
        prepararRoot();

        agregar(
                espacio(10)
        );

        agregar(
                titulo(
                        "WORLD OF WARCRAFT",
                        26
                ),
                -1,
                -2
        );

        TextView subtitle =
                texto(
                        "MOBILE CLIENT",
                        16,
                        GOLD
                );

        subtitle.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        agregar(
                subtitle,
                -1,
                -2
        );

        agregar(
                espacio(24)
        );

        agregar(
                texto(
                        "Selecciona la carpeta raíz de tu cliente\n" +
                        "de World of Warcraft 3.3.5a.",
                        15,
                        MUTED
                ),
                -1,
                -2
        );

        agregar(
                espacio(24)
        );

        Button selectButton =
                boton(
                        "SELECCIONAR CARPETA DEL CLIENTE"
                );

        selectButton.setOnClickListener(
                v -> seleccionarCarpeta()
        );

        agregar(
                selectButton,
                -1,
                -2
        );

        agregar(
                espacio(20)
        );

        statusText =
                texto(
                        "Esperando carpeta...",
                        14,
                        MUTED
                );

        agregar(
                statusText,
                -1,
                -2
        );

        agregar(
                espacio(20)
        );

        agregar(
                texto(
                        "La aplicación descubrirá automáticamente\n" +
                        "las subcarpetas y archivos del cliente.",
                        12,
                        MUTED
                ),
                -1,
                -2
        );
    }

    // ============================================================
    // SELECCIONAR CARPETA
    // ============================================================

    private void seleccionarCarpeta() {

        Intent intent =
                new Intent(
                        Intent.ACTION_OPEN_DOCUMENT_TREE
                );

        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION |
                Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION |
                Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );

        startActivityForResult(
                intent,
                REQUEST_WOW_FOLDER
        );
    }

    private void cargarCarpetaGuardada() {

        SharedPreferences prefs =
                getSharedPreferences(
                        PREFS_NAME,
                        MODE_PRIVATE
                );

        String saved =
                prefs.getString(
                        PREF_WOW_URI,
                        null
                );

        if (saved == null ||
                saved.isEmpty()) {
            return;
        }

        try {

            Uri uri =
                    Uri.parse(saved);

            wowFolderUri = uri;

            try {
                getContentResolver()
                        .takePersistableUriPermission(
                                uri,
                                Intent.FLAG_GRANT_READ_URI_PERMISSION
                        );
            } catch (Exception ignored) {
            }

            nativeSetWowFolder(
                    uri.toString()
            );

            mostrarClienteDetectado();

        } catch (Exception e) {

            statusText.setText(
                    "No se pudo recuperar la carpeta guardada."
            );

            statusText.setTextColor(
                    RED
            );
        }
    }

    @Override
    protected void onActivityResult(
            int requestCode,
            int resultCode,
            Intent data
    ) {
        super.onActivityResult(
                requestCode,
                resultCode,
                data
        );

        if (requestCode != REQUEST_WOW_FOLDER) {
            return;
        }

        if (resultCode != RESULT_OK ||
                data == null) {
            return;
        }

        Uri uri =
                data.getData();

        if (uri == null) {
            return;
        }

        try {

            int flags =
                    data.getFlags() &
                    (
                            Intent.FLAG_GRANT_READ_URI_PERMISSION |
                            Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                    );

            getContentResolver()
                    .takePersistableUriPermission(
                            uri,
                            flags
                    );

        } catch (Exception ignored) {
        }

        wowFolderUri = uri;

        getSharedPreferences(
                PREFS_NAME,
                MODE_PRIVATE
        )
                .edit()
                .putString(
                        PREF_WOW_URI,
                        uri.toString()
                )
                .apply();

        nativeSetWowFolder(
                uri.toString()
        );

        mostrarClienteDetectado();
    }

    // ============================================================
    // CLIENTE DETECTADO
    // ============================================================

    private void mostrarClienteDetectado() {

        prepararRoot();

        agregar(
                espacio(8)
        );

        agregar(
                titulo(
                        "CARPETA DEL CLIENTE",
                        24
                ),
                -1,
                -2
        );

        agregar(
                espacio(8)
        );

        agregar(
                texto(
                        "Raíz seleccionada correctamente",
                        14,
                        GREEN
                ),
                -1,
                -2
        );

        agregar(
                espacio(20)
        );

        LinearLayout panel =
                new LinearLayout(this);

        panel.setOrientation(
                LinearLayout.VERTICAL
        );

        panel.setPadding(
                dp(18),
                dp(15),
                dp(18),
                dp(15)
        );

        panel.setBackgroundColor(
                PANEL
        );

        agregar(
                panel,
                -1,
                -2
        );

        agregarLinea(
                panel,
                "✓",
                "Raíz SAF",
                GREEN
        );

        agregarLinea(
                panel,
                "✓",
                "VFS Android",
                GREEN
        );

        agregarLinea(
                panel,
                "?",
                "Estructura automática",
                BLUE
        );

        agregarLinea(
                panel,
                "?",
                "Archivos y subcarpetas",
                BLUE
        );

        agregar(
                espacio(20)
        );

        scanButton =
                boton(
                        "ESCANEAR CLIENTE"
                );

        scanButton.setOnClickListener(
                v -> ejecutarEscaneo()
        );

        agregar(
                scanButton,
                -1,
                -2
        );

        agregar(
                espacio(12)
        );

        Button change =
                boton(
                        "CAMBIAR CARPETA"
                );

        change.setOnClickListener(
                v -> seleccionarCarpeta()
        );

        agregar(
                change,
                -1,
                -2
        );

        agregar(
                espacio(16)
        );

        agregar(
                texto(
                        "La aplicación no necesita conocer previamente\n" +
                        "los nombres de las subcarpetas.",
                        12,
                        MUTED
                ),
                -1,
                -2
        );
    }

    private void agregarLinea(
            LinearLayout parent,
            String icon,
            String label,
            int color
    ) {
        TextView view =
                texto(
                        icon + "  " + label,
                        14,
                        color
                );

        view.setGravity(
                Gravity.LEFT |
                Gravity.CENTER_VERTICAL
        );

        view.setPadding(
                dp(4),
                dp(7),
                dp(4),
                dp(7)
        );

        parent.addView(
                view,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(38)
                )
        );
    }

    // ============================================================
    // ESCANEAR
    // ============================================================

    private void ejecutarEscaneo() {

        if (wowFolderUri == null) {
            Toast.makeText(
                    this,
                    "Selecciona primero la carpeta del cliente.",
                    Toast.LENGTH_LONG
            ).show();

            return;
        }

        scanButton.setEnabled(false);

        scanButton.setText(
                "ESCANEANDO CLIENTE..."
        );

        String result;

        try {

            result =
                    nativeScanClient();

        } catch (Exception e) {

            result =
                    "SCAN_ERROR\n" +
                    "[ERROR] " +
                    e.getClass().getSimpleName() +
                    "\n" +
                    String.valueOf(
                            e.getMessage()
                    );
        }

        mostrarResultadoEscaneo(
                result
        );
    }

    // ============================================================
    // RESULTADO DEL ESCANEO
    // ============================================================

    private void mostrarResultadoEscaneo(
            String result
    ) {
        prepararRoot();

        agregar(
                espacio(6)
        );

        agregar(
                titulo(
                        "ESCANEO DEL CLIENTE",
                        23
                ),
                -1,
                -2
        );

        agregar(
                espacio(8)
        );

        if (result == null ||
                result.trim().isEmpty()) {

            result =
                    "SCAN_ERROR\n" +
                    "[ERROR] El motor no devolvió resultados.";
        }

        boolean success =
                result.startsWith(
                        "SCAN_OK"
                );

        ScrollView scroll =
                new ScrollView(this);

        LinearLayout content =
                new LinearLayout(this);

        content.setOrientation(
                LinearLayout.VERTICAL
        );

        content.setPadding(
                dp(18),
                dp(14),
                dp(18),
                dp(14)
        );

        content.setBackgroundColor(
                PANEL
        );

        String[] lines =
                result.split("\\n");

        for (String line : lines) {

            TextView row =
                    texto(
                            line,
                            12,
                            WHITE
                    );

            row.setGravity(
                    Gravity.LEFT |
                    Gravity.CENTER_VERTICAL
            );

            row.setPadding(
                    0,
                    dp(3),
                    0,
                    dp(3)
            );

            if (line.startsWith("SCAN_OK")) {

                row.setTextColor(
                        GREEN
                );

                row.setTypeface(
                        Typeface.DEFAULT,
                        Typeface.BOLD
                );

            } else if (
                    line.startsWith("SCAN_ERROR")
            ) {

                row.setTextColor(
                        RED
                );

                row.setTypeface(
                        Typeface.DEFAULT,
                        Typeface.BOLD
                );

            } else if (
                    line.startsWith("[OK]")
            ) {

                row.setTextColor(
                        GREEN
                );

            } else if (
                    line.startsWith("[ERROR]")
            ) {

                row.setTextColor(
                        RED
                );

            } else if (
                    line.startsWith("[DIR]")
            ) {

                row.setTextColor(
                        GOLD_LIGHT
                );

            } else if (
                    line.startsWith("[FILE]")
            ) {

                row.setTextColor(
                        WHITE
                );

            } else if (
                    line.startsWith("[TYPE]")
            ) {

                row.setTextColor(
                        BLUE
                );

            } else if (
                    line.startsWith("[INFO]")
            ) {

                row.setTextColor(
                        MUTED
                );
            }

            content.addView(
                    row
            );
        }

        scroll.addView(
                content
        );

        rootLayout.addView(
                scroll,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        0,
                        1.0f
                )
        );

        agregar(
                espacio(10)
        );

        continueLoginButton =
                boton(
                        "CONTINUAR AL LOGIN"
                );

        continueLoginButton.setEnabled(
                success
        );

        continueLoginButton.setAlpha(
                success
                        ? 1.0f
                        : 0.45f
        );

        continueLoginButton.setOnClickListener(
                v -> mostrarLogin()
        );

        agregar(
                continueLoginButton,
                -1,
                -2
        );

        agregar(
                espacio(8)
        );

        Button rescan =
                boton(
                        "VOLVER A ESCANEAR"
                );

        rescan.setOnClickListener(
                v -> ejecutarEscaneo()
        );

        agregar(
                rescan,
                -1,
                -2
        );

        agregar(
                espacio(8)
        );

        Button change =
                boton(
                        "CAMBIAR CARPETA"
                );

        change.setOnClickListener(
                v -> seleccionarCarpeta()
        );

        agregar(
                change,
                -1,
                -2
        );
    }

    // ============================================================
    // LOGIN
    // ============================================================

    private void mostrarLogin() {

        prepararRoot();

        agregar(
                espacio(10)
        );

        agregar(
                titulo(
                        "WORLD OF WARCRAFT",
                        25
                ),
                -1,
                -2
        );

        TextView subtitle =
                texto(
                        "INICIAR SESIÓN",
                        15,
                        GOLD
                );

        subtitle.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        agregar(
                subtitle,
                -1,
                -2
        );

        agregar(
                espacio(28)
        );

        usernameInput =
                new EditText(this);

        usernameInput.setHint(
                "Usuario"
        );

        usernameInput.setHintTextColor(
                MUTED
        );

        usernameInput.setTextColor(
                WHITE
        );

        usernameInput.setTextSize(
                16
        );

        usernameInput.setSingleLine(
                true
        );

        usernameInput.setPadding(
                dp(16),
                0,
                dp(16),
                0
        );

        usernameInput.setBackgroundColor(
                PANEL_2
        );

        agregar(
                usernameInput,
                -1,
                56
        );

        agregar(
                espacio(14)
        );

        passwordInput =
                new EditText(this);

        passwordInput.setHint(
                "Contraseña"
        );

        passwordInput.setHintTextColor(
                MUTED
        );

        passwordInput.setTextColor(
                WHITE
        );

        passwordInput.setTextSize(
                16
        );

        passwordInput.setSingleLine(
                true
        );

        passwordInput.setInputType(
                InputType.TYPE_CLASS_TEXT |
                InputType.TYPE_TEXT_VARIATION_PASSWORD
        );

        passwordInput.setPadding(
                dp(16),
                0,
                dp(16),
                0
        );

        passwordInput.setBackgroundColor(
                PANEL_2
        );

        agregar(
                passwordInput,
                -1,
                56
        );

        agregar(
                espacio(22)
        );

        Button login =
                boton(
                        "CONECTAR"
                );

        login.setOnClickListener(
                v -> realizarLogin()
        );

        agregar(
                login,
                -1,
                -2
        );

        agregar(
                espacio(14)
        );

        statusText =
                texto(
                        "Protocolo WoW 3.3.5a pendiente.",
                        13,
                        BLUE
                );

        agregar(
                statusText,
                -1,
                -2
        );

        agregar(
                espacio(18)
        );

        Button back =
                boton(
                        "VOLVER"
                );

        back.setOnClickListener(
                v -> mostrarClienteDetectado()
        );

        agregar(
                back,
                -1,
                -2
        );
    }

    private void realizarLogin() {

        String username =
                usernameInput
                        .getText()
                        .toString()
                        .trim();

        String password =
                passwordInput
                        .getText()
                        .toString();

        if (username.isEmpty()) {

            usernameInput.setError(
                    "Introduce el usuario."
            );

            usernameInput.requestFocus();

            return;
        }

        if (password.isEmpty()) {

            passwordInput.setError(
                    "Introduce la contraseña."
            );

            passwordInput.requestFocus();

            return;
        }

        statusText.setText(
                "Enviando solicitud de autenticación..."
        );

        statusText.setTextColor(
                BLUE
        );

        nativeLogin(
                username,
                password
        );
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

        while (
                normalized.startsWith("/")
        ) {
            normalized =
                    normalized.substring(1);
        }

        String[] parts =
                normalized.split("/");

        for (String part : parts) {

            if (part.isEmpty()) {
                continue;
            }

            DocumentFile next =
                    null;

            for (
                    DocumentFile child :
                    current.listFiles()
            ) {

                String name =
                        child.getName();

                if (name != null &&
                        part.equals(name)) {

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
            String path
    ) {
        return encontrarDocumento(path) != null;
    }

    private boolean vfsIsDirectory(
            String path
    ) {
        DocumentFile file =
                encontrarDocumento(path);

        return file != null &&
                file.isDirectory();
    }

    private List<String> vfsList(
            String path
    ) {
        List<String> result =
                new ArrayList<>();

        DocumentFile directory =
                encontrarDocumento(path);

        if (directory == null ||
                !directory.isDirectory()) {

            return result;
        }

        for (
                DocumentFile file :
                directory.listFiles()
        ) {

            String name =
                    file.getName();

            if (name == null) {
                continue;
            }

            if (file.isDirectory()) {
                result.add(
                        name + "/"
                );
            } else {
                result.add(
                        name
                );
            }
        }

        return result;
    }

    private byte[] vfsReadFile(
            String path
    ) {
        DocumentFile file =
                encontrarDocumento(path);

        if (file == null ||
                !file.isFile()) {

            return null;
        }

        try {

            long length =
                    file.length();

            if (
                    length >
                    16L * 1024L * 1024L
            ) {
                return null;
            }

            InputStream input =
                    getContentResolver()
                            .openInputStream(
                                    file.getUri()
                            );

            if (input == null) {
                return null;
            }

            try {

                ByteArrayOutputStream output =
                        new ByteArrayOutputStream();

                byte[] buffer =
                        new byte[8192];

                int read;

                while (
                        (read =
                                input.read(buffer))
                                != -1
                ) {

                    output.write(
                            buffer,
                            0,
                            read
                    );
                }

                return output.toByteArray();

            } finally {

                input.close();
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

            float x =
                    event.getX();

            float y =
                    event.getY();

            if (
                    action ==
                    MotionEvent.ACTION_DOWN
            ) {

                nativeTouchDown(
                        x,
                        y
                );

            } else if (
                    action ==
                    MotionEvent.ACTION_MOVE
            ) {

                nativeTouchMove(
                        x,
                        y
                );

            } else if (
                    action ==
                    MotionEvent.ACTION_UP ||
                    action ==
                    MotionEvent.ACTION_CANCEL
            ) {

                nativeTouchUp(
                        x,
                        y
                );
            }

        } catch (Exception ignored) {
        }

        return super.dispatchTouchEvent(
                event
        );
    }

    // ============================================================
    // UTILIDADES
    // ============================================================

    private int dp(int value) {

        float density =
                getResources()
                        .getDisplayMetrics()
                        .density;

        return Math.round(
                value * density
        );
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
