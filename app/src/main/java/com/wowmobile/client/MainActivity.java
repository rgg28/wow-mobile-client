package com.wowmobile.client;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.documentfile.provider.DocumentFile;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class MainActivity extends Activity {

    private static final int REQUEST_FOLDER = 1001;
    private static final String PREFS = "wow_mobile";
    private static final String PREF_URI = "wow_root_uri";

    private Uri wowRootUri;

    private LinearLayout rootLayout;
    private TextView statusText;
    private TextView contentText;

    private Button selectFolderButton;
    private Button continueButton;

    private LinearLayout loginPanel;
    private EditText usernameEdit;
    private EditText passwordEdit;

    static {
        System.loadLibrary("wowmobile");
    }

    private native void nativeInit();

    private native void nativeSetWowFolder(String uri);

    private native String nativeInspectRoot();

    private native String nativeListDirectory(
            String relativePath
    );

    private native String nativeReadResource(
            String relativePath
    );

    private native String nativeLogin(
            String username,
            String password
    );

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        nativeInit();

        buildMainUI();

        loadSavedFolder();
    }

    // =========================================================
    // UI
    // =========================================================

    private void buildMainUI() {

        rootLayout =
                new LinearLayout(this);

        rootLayout.setOrientation(
                LinearLayout.VERTICAL
        );

        rootLayout.setPadding(
                dp(24),
                dp(20),
                dp(24),
                dp(20)
        );

        rootLayout.setBackgroundColor(
                Color.rgb(7, 12, 24)
        );

        setContentView(rootLayout);

        TextView title =
                new TextView(this);

        title.setText(
                "WORLD OF WARCRAFT"
        );

        title.setTextColor(
                Color.rgb(236, 191, 82)
        );

        title.setTextSize(26);

        title.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        title.setGravity(
                Gravity.CENTER
        );

        rootLayout.addView(
                title,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(50)
                )
        );

        TextView subtitle =
                new TextView(this);

        subtitle.setText(
                "3.3.5a • BUILD 12340 • MOBILE CLIENT"
        );

        subtitle.setTextColor(
                Color.rgb(150, 165, 190)
        );

        subtitle.setTextSize(13);

        subtitle.setGravity(
                Gravity.CENTER
        );

        rootLayout.addView(
                subtitle,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(35)
                )
        );

        statusText =
                new TextView(this);

        statusText.setText(
                "Selecciona la carpeta raíz del cliente."
        );

        statusText.setTextColor(
                Color.WHITE
        );

        statusText.setTextSize(16);

        statusText.setGravity(
                Gravity.CENTER
        );

        statusText.setPadding(
                dp(10),
                dp(15),
                dp(10),
                dp(15)
        );

        rootLayout.addView(
                statusText,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );

        contentText =
                new TextView(this);

        contentText.setTextColor(
                Color.rgb(190, 200, 220)
        );

        contentText.setTextSize(14);

        contentText.setPadding(
                dp(15),
                dp(15),
                dp(15),
                dp(15)
        );

        contentText.setTextIsSelectable(true);

        ScrollView scroll =
                new ScrollView(this);

        scroll.setBackgroundColor(
                Color.rgb(12, 20, 37)
        );

        scroll.addView(contentText);

        LinearLayout.LayoutParams scrollParams =
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        0
                );

        scrollParams.weight = 1;

        rootLayout.addView(
                scroll,
                scrollParams
        );

        selectFolderButton =
                createButton(
                        "SELECCIONAR CARPETA"
                );

        rootLayout.addView(
                selectFolderButton,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(55)
                )
        );

        selectFolderButton.setOnClickListener(
                v -> selectWowFolder()
        );

        continueButton =
                createButton(
                        "CONTINUAR"
                );

        rootLayout.addView(
                continueButton,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(55)
                )
        );

        continueButton.setVisibility(
                View.GONE
        );

        continueButton.setOnClickListener(
                v -> showLogin()
        );

        createLoginPanel();

        loginPanel.setVisibility(
                View.GONE
        );
    }

    private Button createButton(
            String text
    ) {

        Button button =
                new Button(this);

        button.setText(text);

        button.setTextColor(
                Color.rgb(10, 15, 25)
        );

        button.setTextSize(15);

        button.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        button.setAllCaps(false);

        button.setBackgroundColor(
                Color.rgb(205, 166, 68)
        );

        return button;
    }

    // =========================================================
    // LOGIN
    // =========================================================

    private void createLoginPanel() {

        loginPanel =
                new LinearLayout(this);

        loginPanel.setOrientation(
                LinearLayout.VERTICAL
        );

        loginPanel.setPadding(
                dp(10),
                dp(15),
                dp(10),
                dp(10)
        );

        TextView loginTitle =
                new TextView(this);

        loginTitle.setText(
                "CUENTA"
        );

        loginTitle.setTextColor(
                Color.rgb(236, 191, 82)
        );

        loginTitle.setTextSize(20);

        loginTitle.setGravity(
                Gravity.CENTER
        );

        loginPanel.addView(
                loginTitle,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(45)
                )
        );

        usernameEdit =
                new EditText(this);

        usernameEdit.setHint(
                "Usuario"
        );

        usernameEdit.setSingleLine(
                true
        );

        usernameEdit.setTextColor(
                Color.WHITE
        );

        usernameEdit.setHintTextColor(
                Color.rgb(120, 130, 150)
        );

        loginPanel.addView(
                usernameEdit,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(55)
                )
        );

        passwordEdit =
                new EditText(this);

        passwordEdit.setHint(
                "Contraseña"
        );

        passwordEdit.setSingleLine(
                true
        );

        passwordEdit.setInputType(
                android.text.InputType.TYPE_CLASS_TEXT |
                android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
        );

        passwordEdit.setTextColor(
                Color.WHITE
        );

        passwordEdit.setHintTextColor(
                Color.rgb(120, 130, 150)
        );

        loginPanel.addView(
                passwordEdit,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(55)
                )
        );

        Button loginButton =
                createButton(
                        "CONECTAR"
                );

        loginPanel.addView(
                loginButton,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        dp(55)
                )
        );

        loginButton.setOnClickListener(
                v -> performLogin()
        );

        rootLayout.addView(
                loginPanel,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    private void showLogin() {

        continueButton.setVisibility(
                View.GONE
        );

        selectFolderButton.setVisibility(
                View.GONE
        );

        contentText.setText(
                "CLIENTE 3.3.5a PREPARADO\n\n" +
                "Recursos externos:\n" +
                "BLP • M2 • SKIN • ANIM • SBT\n\n" +
                "Los archivos se abrirán bajo demanda.\n\n" +
                "Introduce tus credenciales."
        );

        statusText.setText(
                "INICIAR SESIÓN"
        );

        loginPanel.setVisibility(
                View.VISIBLE
        );
    }

    private void performLogin() {

        String username =
                usernameEdit
                        .getText()
                        .toString()
                        .trim();

        String password =
                passwordEdit
                        .getText()
                        .toString();

        if (username.isEmpty()) {

            usernameEdit.setError(
                    "Introduce el usuario"
            );

            return;
        }

        if (password.isEmpty()) {

            passwordEdit.setError(
                    "Introduce la contraseña"
            );

            return;
        }

        statusText.setText(
                "CONECTANDO..."
        );

        new Thread(() -> {

            String result;

            try {

                result =
                        nativeLogin(
                                username,
                                password
                        );

            } catch (Exception e) {

                result =
                        "ERROR: " +
                        e.getMessage();
            }

            final String finalResult =
                    result;

            runOnUiThread(() -> {

                contentText.setText(
                        finalResult
                );

                if (finalResult.contains(
                        "NO IMPLEMENTADO"
                )) {

                    statusText.setText(
                            "PROTOCOLO PENDIENTE"
                    );

                } else {

                    statusText.setText(
                            "ESTADO DE CONEXIÓN"
                    );
                }
            });

        }).start();
    }

    // =========================================================
    // SAF
    // =========================================================

    private void selectWowFolder() {

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
                REQUEST_FOLDER
        );
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

        if (requestCode != REQUEST_FOLDER ||
                resultCode != RESULT_OK ||
                data == null ||
                data.getData() == null) {

            return;
        }

        Uri uri =
                data.getData();

        try {

            int flags =
                    data.getFlags() &
                    (Intent.FLAG_GRANT_READ_URI_PERMISSION |
                     Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

            getContentResolver()
                    .takePersistableUriPermission(
                            uri,
                            flags
                    );

        } catch (Exception ignored) {
        }

        wowRootUri = uri;

        getSharedPreferences(
                PREFS,
                MODE_PRIVATE
        )
        .edit()
        .putString(
                PREF_URI,
                uri.toString()
        )
        .apply();

        nativeSetWowFolder(
                uri.toString()
        );

        inspectRootAsync();
    }

    private void loadSavedFolder() {

        String saved =
                getSharedPreferences(
                        PREFS,
                        MODE_PRIVATE
                )
                .getString(
                        PREF_URI,
                        null
                );

        if (saved == null ||
                saved.isEmpty()) {

            return;
        }

        try {

            wowRootUri =
                    Uri.parse(saved);

            DocumentFile root =
                    DocumentFile.fromTreeUri(
                            this,
                            wowRootUri
                    );

            if (root == null ||
                    !root.canRead()) {

                return;
            }

            nativeSetWowFolder(
                    wowRootUri.toString()
            );

            inspectRootAsync();

        } catch (Exception e) {

            contentText.setText(
                    "No se pudo abrir la carpeta guardada.\n\n" +
                    e.getMessage()
            );
        }
    }

    // =========================================================
    // INSPECCIÓN
    // =========================================================

    private void inspectRootAsync() {

        statusText.setText(
                "ANALIZANDO CLIENTE..."
        );

        contentText.setText(
                "Comprobando estructura...\n\n" +
                "No se realizará un escaneo recursivo."
        );

        new Thread(() -> {

            String result;

            try {

                result =
                        nativeInspectRoot();

            } catch (Exception e) {

                result =
                        "ERROR:\n" +
                        e.getMessage();
            }

            final String finalResult =
                    result;

            runOnUiThread(() -> {

                contentText.setText(
                        finalResult
                );

                continueButton.setVisibility(
                        View.VISIBLE
                );

                statusText.setText(
                        finalResult.startsWith(
                                "CLIENTE DETECTADO"
                        )
                        ? "CLIENTE 3.3.5a DETECTADO"
                        : "CARPETA SELECCIONADA"
                );
            });

        }).start();
    }

    // =========================================================
    // VFS
    // =========================================================

    public String vfsList(
            String relativePath
    ) {

        if (wowRootUri == null) {
            return "";
        }

        try {

            DocumentFile current =
                    DocumentFile.fromTreeUri(
                            this,
                            wowRootUri
                    );

            if (current == null) {
                return "";
            }

            if (relativePath != null &&
                    !relativePath.isEmpty()) {

                String[] parts =
                        relativePath.split("/");

                for (String part : parts) {

                    if (part == null ||
                            part.isEmpty()) {
                        continue;
                    }

                    DocumentFile next =
                            null;

                    for (DocumentFile child :
                            current.listFiles()) {

                        String name =
                                child.getName();

                        if (name != null &&
                                name.equals(part)) {

                            next = child;
                            break;
                        }
                    }

                    if (next == null ||
                            !next.isDirectory()) {

                        return "";
                    }

                    current = next;
                }
            }

            StringBuilder result =
                    new StringBuilder();

            for (DocumentFile child :
                    current.listFiles()) {

                String name =
                        child.getName();

                if (name == null) {
                    continue;
                }

                if (child.isDirectory()) {

                    result
                            .append("D|")
                            .append(name)
                            .append('\n');

                } else {

                    result
                            .append("F|")
                            .append(name)
                            .append('\n');
                }
            }

            return result.toString();

        } catch (Exception e) {

            return "";
        }
    }

    // =========================================================
    // LECTURA BINARIA
    // =========================================================

    public byte[] vfsReadFile(
            String relativePath,
            int maxBytes
    ) {

        if (wowRootUri == null) {
            return null;
        }

        try {

            DocumentFile file =
                    findDocument(
                            relativePath
                    );

            if (file == null ||
                    !file.isFile() ||
                    !file.canRead()) {

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

            ByteArrayOutputStream output =
                    new ByteArrayOutputStream();

            byte[] buffer =
                    new byte[8192];

            int total = 0;

            int read;

            while ((read =
                    input.read(buffer)) != -1) {

                if (total + read >
                        maxBytes) {

                    int allowed =
                            maxBytes - total;

                    if (allowed > 0) {

                        output.write(
                                buffer,
                                0,
                                allowed
                        );
                    }

                    break;
                }

                output.write(
                        buffer,
                        0,
                        read
                );

                total += read;
            }

            input.close();

            return output.toByteArray();

        } catch (Exception e) {

            return null;
        }
    }

    private DocumentFile findDocument(
            String relativePath
    ) {

        if (wowRootUri == null) {
            return null;
        }

        DocumentFile current =
                DocumentFile.fromTreeUri(
                        this,
                        wowRootUri
                );

        if (current == null) {
            return null;
        }

        String clean =
                relativePath == null
                ? ""
                : relativePath
                    .replace('\\', '/');

        String[] parts =
                clean.split("/");

        for (String part : parts) {

            if (part == null ||
                    part.isEmpty()) {

                continue;
            }

            DocumentFile next =
                    null;

            for (DocumentFile child :
                    current.listFiles()) {

                String name =
                        child.getName();

                if (name != null &&
                        name.equals(part)) {

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

    // =========================================================
    // LECTOR DE RECURSO
    // =========================================================

    public String readResource(
            String relativePath
    ) {

        try {

            byte[] data =
                    vfsReadFile(
                            relativePath,
                            1024 * 1024
                    );

            if (data == null) {

                return
                        "ERROR\n\n" +
                        "No se pudo abrir:\n" +
                        relativePath;
            }

            return
                    "RESOURCE\n\n" +
                    "Ruta: " +
                    relativePath +
                    "\n" +
                    "Bytes leídos: " +
                    data.length;

        } catch (Exception e) {

            return
                    "ERROR\n\n" +
                    e.getMessage();
        }
    }

    // =========================================================
    // UTILIDADES
    // =========================================================

    private int dp(int value) {

        float density =
                getResources()
                        .getDisplayMetrics()
                        .density;

        return (int)
                (value * density + 0.5f);
    }

    @Override
    public void onBackPressed() {

        if (loginPanel != null &&
                loginPanel.getVisibility() ==
                        View.VISIBLE) {

            loginPanel.setVisibility(
                    View.GONE
            );

            selectFolderButton.setVisibility(
                    View.VISIBLE
            );

            continueButton.setVisibility(
                    View.VISIBLE
            );

            statusText.setText(
                    "CLIENTE DETECTADO"
            );

            return;
        }

        super.onBackPressed();
    }
}
