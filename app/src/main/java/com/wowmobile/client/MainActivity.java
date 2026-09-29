package com.wowmobile.client;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Space;
import android.widget.TextView;
import android.widget.Toast;

import androidx.documentfile.provider.DocumentFile;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends Activity {

    private static final int REQUEST_WOW_FOLDER = 1001;

    // ============================================================
    // COLORES
    // ============================================================

    private static final int BACK_TOP = Color.rgb(8, 14, 27);
    private static final int BACK_BOTTOM = Color.rgb(18, 29, 48);

    private static final int PANEL = Color.rgb(22, 30, 45);
    private static final int PANEL_LIGHT = Color.rgb(31, 42, 61);
    private static final int PANEL_DARK = Color.rgb(14, 20, 32);

    private static final int GOLD = Color.rgb(220, 173, 74);
    private static final int GOLD_LIGHT = Color.rgb(245, 205, 105);

    private static final int TEXT = Color.rgb(242, 242, 242);
    private static final int SUBTEXT = Color.rgb(166, 177, 195);

    private static final int GREEN = Color.rgb(82, 190, 105);
    private static final int RED = Color.rgb(210, 82, 82);
    private static final int BLUE = Color.rgb(70, 140, 220);

    // ============================================================
    // UI
    // ============================================================

    private LinearLayout root;

    private TextView status;

    private EditText usernameInput;
    private EditText passwordInput;

    private Button loginButton;

    private Uri wowFolderUri;

    // ============================================================
    // NATIVE
    // ============================================================

    static {
        System.loadLibrary("wowmobile");
    }

    private native void nativeInit();

    private native void nativeSetWowFolder(String path);

    private native void nativeTestVfs();

    private native void nativeLogin(
            String username,
            String password
    );

    private native void nativeSelectCharacter(int index);

    private native void nativeTouch(
            int action,
            float x,
            float y
    );

    private native void nativeJoystick(
            float x,
            float y
    );

    private native void nativeSpell(
            int spellId
    );

    private native void nativeJump();

    // ============================================================
    // ACTIVITY
    // ============================================================

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        nativeInit();

        crearBase();

        Uri saved = obtenerCarpetaGuardada();

        if (saved != null && validarCliente(saved)) {

            wowFolderUri = saved;

            nativeSetWowFolder(saved.toString());

            mostrarClienteEncontrado(saved);

        } else {

            mostrarSeleccionCliente();
        }
    }

    // ============================================================
    // BASE
    // ============================================================

    private void crearBase() {

        root = new LinearLayout(this);

        root.setOrientation(
                LinearLayout.VERTICAL
        );

        root.setGravity(
                Gravity.CENTER
        );

        root.setPadding(
                25,
                25,
                25,
                25
        );

        GradientDrawable background =
                new GradientDrawable(
                        GradientDrawable.Orientation.TL_BR,
                        new int[]{
                                BACK_TOP,
                                BACK_BOTTOM
                        }
                );

        root.setBackground(background);

        setContentView(root);
    }

    private void limpiar() {
        root.removeAllViews();
    }

    // ============================================================
    // TEXTOS
    // ============================================================

    private TextView titulo(
            String texto,
            float tamaño
    ) {

        TextView v = new TextView(this);

        v.setText(texto);
        v.setTextColor(GOLD_LIGHT);
        v.setTextSize(tamaño);
        v.setGravity(Gravity.CENTER);

        v.setTypeface(
                Typeface.create(
                        Typeface.DEFAULT,
                        Typeface.BOLD
                )
        );

        return v;
    }

    private TextView subtitulo(
            String texto
    ) {

        TextView v = new TextView(this);

        v.setText(texto);
        v.setTextColor(SUBTEXT);
        v.setTextSize(15);
        v.setGravity(Gravity.CENTER);

        v.setPadding(
                15,
                6,
                15,
                6
        );

        return v;
    }

    private TextView etiqueta(
            String texto
    ) {

        TextView v = new TextView(this);

        v.setText(texto);
        v.setTextColor(GOLD);
        v.setTextSize(14);

        v.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        v.setPadding(
                4,
                5,
                4,
                5
        );

        return v;
    }

    // ============================================================
    // PANEL
    // ============================================================

    private LinearLayout crearPanel() {

        LinearLayout panel =
                new LinearLayout(this);

        panel.setOrientation(
                LinearLayout.VERTICAL
        );

        panel.setGravity(
                Gravity.CENTER
        );

        panel.setPadding(
                38,
                35,
                38,
                35
        );

        GradientDrawable fondo =
                new GradientDrawable();

        fondo.setColor(PANEL);
        fondo.setCornerRadius(22);
        fondo.setStroke(
                2,
                Color.rgb(75, 91, 116)
        );

        panel.setBackground(fondo);

        return panel;
    }

    // ============================================================
    // BOTONES
    // ============================================================

    private Button crearBoton(
            String texto
    ) {

        Button b = new Button(this);

        b.setText(texto);
        b.setTextColor(TEXT);
        b.setTextSize(15);
        b.setAllCaps(false);
        b.setGravity(Gravity.CENTER);

        b.setTypeface(
                Typeface.DEFAULT,
                Typeface.BOLD
        );

        b.setPadding(
                20,
                5,
                20,
                5
        );

        GradientDrawable fondo =
                new GradientDrawable();

        fondo.setColor(PANEL_LIGHT);
        fondo.setCornerRadius(14);
        fondo.setStroke(
                1,
                Color.rgb(91, 107, 135)
        );

        b.setBackground(fondo);

        return b;
    }

    private Button crearBotonDorado(
            String texto
    ) {

        Button b = crearBoton(texto);

        GradientDrawable fondo =
                new GradientDrawable();

        fondo.setColor(
                Color.rgb(111, 78, 25)
        );

        fondo.setCornerRadius(14);

        fondo.setStroke(
                2,
                GOLD
        );

        b.setBackground(fondo);
        b.setTextColor(GOLD_LIGHT);

        return b;
    }

    // ============================================================
    // ESPACIADO
    // ============================================================

    private Space espacio(
            int altura
    ) {

        Space s = new Space(this);

        s.setLayoutParams(
                new LinearLayout.LayoutParams(
                        1,
                        altura
                )
        );

        return s;
    }

    // ============================================================
    // CABECERA
    // ============================================================

    private void agregarCabecera(
            LinearLayout panel
    ) {

        panel.addView(
                titulo(
                        "WORLD OF WARCRAFT",
                        25
                )
        );

        panel.addView(
                subtitulo(
                        "MOBILE CLIENT"
                )
        );

        TextView linea =
                new TextView(this);

        linea.setBackgroundColor(GOLD);

        LinearLayout.LayoutParams lp =
                new LinearLayout.LayoutParams(
                        300,
                        2
                );

        lp.setMargins(
                0,
                12,
                0,
                15
        );

        panel.addView(
                linea,
                lp
        );
    }

    // ============================================================
    // SELECCION CLIENTE
    // ============================================================

    private void mostrarSeleccionCliente() {

        limpiar();

        LinearLayout panel =
                crearPanel();

        agregarCabecera(panel);

        panel.addView(
                titulo(
                        "Cliente de juego",
                        21
                )
        );

        panel.addView(
                espacio(12)
        );

        panel.addView(
                subtitulo(
                        "Selecciona la carpeta donde se encuentran\n" +
                        "los archivos extraídos de WoW 3.3.5a."
                )
        );

        panel.addView(
                espacio(22)
        );

        Button seleccionar =
                crearBotonDorado(
                        "📁  SELECCIONAR CLIENTE"
                );

        seleccionar.setOnClickListener(
                v -> abrirSelectorCarpeta()
        );

        panel.addView(
                seleccionar,
                new LinearLayout.LayoutParams(
                        360,
                        65
                )
        );

        panel.addView(
                espacio(18)
        );

        status =
                subtitulo(
                        "●  Ningún cliente seleccionado"
                );

        panel.addView(status);

        panel.addView(
                espacio(12)
        );

        panel.addView(
                subtitulo(
                        "Los archivos permanecen en el almacenamiento\n" +
                        "del dispositivo y no se incluyen dentro del APK."
                )
        );

        root.addView(
                panel,
                new LinearLayout.LayoutParams(
                        620,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    // ============================================================
    // SAF
    // ============================================================

    private void abrirSelectorCarpeta() {

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

            mostrarSeleccionCliente();
            return;
        }

        Uri uri = data.getData();

        if (uri == null) {

            mostrarSeleccionCliente();
            return;
        }

        try {

            getContentResolver()
                    .takePersistableUriPermission(
                            uri,
                            Intent.FLAG_GRANT_READ_URI_PERMISSION
                    );

        } catch (Exception ignored) {
        }

        wowFolderUri = uri;

        guardarCarpeta(uri);

        nativeSetWowFolder(
                uri.toString()
        );

        if (validarCliente(uri)) {

            mostrarClienteEncontrado(uri);

        } else {

            mostrarClienteNoValido(uri);
        }
    }

    // ============================================================
    // VALIDACION CLIENTE
    // ============================================================

    private boolean validarCliente(
            Uri uri
    ) {

        DocumentFile rootFolder =
                DocumentFile.fromTreeUri(
                        this,
                        uri
                );

        if (rootFolder == null ||
                !rootFolder.isDirectory()) {

            return false;
        }

        for (DocumentFile file :
                rootFolder.listFiles()) {

            String nombre =
                    file.getName();

            if (nombre == null) {
                continue;
            }

            if (!file.isDirectory()) {
                continue;
            }

            if (nombre.equalsIgnoreCase("cameras") ||
                    nombre.equalsIgnoreCase("character") ||
                    nombre.equalsIgnoreCase("creatures")) {

                return true;
            }
        }

        return false;
    }

    private boolean existeCarpeta(
            Uri uri,
            String nombre
    ) {

        DocumentFile rootFolder =
                DocumentFile.fromTreeUri(
                        this,
                        uri
                );

        if (rootFolder == null) {
            return false;
        }

        for (DocumentFile file :
                rootFolder.listFiles()) {

            String n =
                    file.getName();

            if (n != null &&
                    n.equalsIgnoreCase(nombre) &&
                    file.isDirectory()) {

                return true;
            }
        }

        return false;
    }

    // ============================================================
    // CLIENTE ENCONTRADO
    // ============================================================

    private void mostrarClienteEncontrado(
            Uri uri
    ) {

        limpiar();

        LinearLayout panel =
                crearPanel();

        agregarCabecera(panel);

        panel.addView(
                titulo(
                        "Cliente detectado",
                        22
                )
        );

        panel.addView(
                espacio(8)
        );

        panel.addView(
                subtitulo(
                        "La estructura del cliente fue encontrada."
                )
        );

        panel.addView(
                espacio(18)
        );

        agregarResultado(
                panel,
                "Cameras",
                existeCarpeta(uri, "cameras")
        );

        agregarResultado(
                panel,
                "Character",
                existeCarpeta(uri, "character")
        );

        agregarResultado(
                panel,
                "Creatures",
                existeCarpeta(uri, "creatures")
        );

        panel.addView(
                espacio(15)
        );

        agregarResultado(
                panel,
                "SAF / almacenamiento",
                true
        );

        panel.addView(
                espacio(10)
        );

        TextView vfs =
                subtitulo(
                        "VFS Android preparado\n" +
                        "Los archivos serán leídos directamente desde el cliente."
                );

        vfs.setTextColor(BLUE);

        panel.addView(vfs);

        panel.addView(
                espacio(18)
        );

        Button probar =
                crearBoton(
                        "PROBAR LECTURA DEL CLIENTE"
                );

        probar.setOnClickListener(
                v -> {

                    statusVfs();

                    try {
                        nativeTestVfs();
                    } catch (Exception ignored) {
                    }
                }
        );

        panel.addView(
                probar,
                new LinearLayout.LayoutParams(
                        360,
                        60
                )
        );

        panel.addView(
                espacio(10)
        );

        Button continuar =
                crearBotonDorado(
                        "CONTINUAR AL LOGIN"
                );

        continuar.setOnClickListener(
                v -> mostrarLogin()
        );

        panel.addView(
                continuar,
                new LinearLayout.LayoutParams(
                        360,
                        65
                )
        );

        panel.addView(
                espacio(10)
        );

        Button cambiar =
                crearBoton(
                        "Cambiar carpeta"
                );

        cambiar.setOnClickListener(
                v -> abrirSelectorCarpeta()
        );

        panel.addView(cambiar);

        root.addView(
                panel,
                new LinearLayout.LayoutParams(
                        620,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    private void statusVfs() {

        Toast.makeText(
                this,
                "Probando lectura SAF...",
                Toast.LENGTH_SHORT
        ).show();
    }

    private void agregarResultado(
            LinearLayout panel,
            String nombre,
            boolean encontrado
    ) {

        TextView v =
                subtitulo(
                        (encontrado ? "✓  " : "○  ") +
                        nombre
                );

        v.setGravity(
                Gravity.LEFT |
                Gravity.CENTER_VERTICAL
        );

        v.setTextColor(
                encontrado ?
                        GREEN :
                        SUBTEXT
        );

        panel.addView(v);
    }

    // ============================================================
    // CLIENTE INVALIDO
    // ============================================================

    private void mostrarClienteNoValido(
            Uri uri
    ) {

        limpiar();

        LinearLayout panel =
                crearPanel();

        agregarCabecera(panel);

        panel.addView(
                titulo(
                        "Cliente no reconocido",
                        22
                )
        );

        panel.addView(
                espacio(12)
        );

        panel.addView(
                subtitulo(
                        "La carpeta seleccionada no parece ser\n" +
                        "la raíz del cliente WoW extraído."
                )
        );

        panel.addView(
                espacio(18)
        );

        TextView error =
                subtitulo(
                        "✕  No se encontraron cameras,\n" +
                        "   character o creatures."
                );

        error.setTextColor(RED);

        panel.addView(error);

        panel.addView(
                espacio(20)
        );

        Button retry =
                crearBotonDorado(
                        "SELECCIONAR OTRA CARPETA"
                );

        retry.setOnClickListener(
                v -> abrirSelectorCarpeta()
        );

        panel.addView(
                retry,
                new LinearLayout.LayoutParams(
                        380,
                        65
                )
        );

        root.addView(
                panel,
                new LinearLayout.LayoutParams(
                        620,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    // ============================================================
    // LOGIN
    // ============================================================

    private void mostrarLogin() {

        limpiar();

        LinearLayout panel =
                crearPanel();

        agregarCabecera(panel);

        panel.addView(
                titulo(
                        "Autenticación",
                        22
                )
        );

        panel.addView(
                espacio(15)
        );

        panel.addView(
                etiqueta("USUARIO")
        );

        usernameInput =
                new EditText(this);

        prepararCampo(
                usernameInput,
                "Nombre de usuario"
        );

        panel.addView(
                usernameInput,
                new LinearLayout.LayoutParams(
                        440,
                        60
                )
        );

        panel.addView(
                espacio(12)
        );

        panel.addView(
                etiqueta("CONTRASEÑA")
        );

        passwordInput =
                new EditText(this);

        prepararCampo(
                passwordInput,
                "Contraseña"
        );

        passwordInput.setInputType(
                android.text.InputType.TYPE_CLASS_TEXT |
                android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
        );

        panel.addView(
                passwordInput,
                new LinearLayout.LayoutParams(
                        440,
                        60
                )
        );

        panel.addView(
                espacio(22)
        );

        loginButton =
                crearBotonDorado(
                        "CONECTAR"
                );

        loginButton.setOnClickListener(
                v -> intentarLogin()
        );

        panel.addView(
                loginButton,
                new LinearLayout.LayoutParams(
                        360,
                        65
                )
        );

        panel.addView(
                espacio(12)
        );

        status =
                subtitulo(
                        "●  Cliente cargado\n" +
                        "●  VFS disponible\n" +
                        "●  Conexión no iniciada"
                );

        panel.addView(status);

        panel.addView(
                espacio(12)
        );

        Button volver =
                crearBoton(
                        "Volver"
                );

        volver.setOnClickListener(
                v -> mostrarClienteEncontrado(
                        wowFolderUri
                )
        );

        panel.addView(volver);

        root.addView(
                panel,
                new LinearLayout.LayoutParams(
                        620,
                        ViewGroup.LayoutParams.WRAP_CONTENT
                )
        );
    }

    private void prepararCampo(
            EditText campo,
            String hint
    ) {

        campo.setSingleLine(true);

        campo.setTextColor(TEXT);

        campo.setHintTextColor(
                Color.rgb(
                        120,
                        130,
                        145
                )
        );

        campo.setHint(hint);

        campo.setTextSize(17);

        campo.setPadding(
                18,
                5,
                18,
                5
        );

        GradientDrawable fondo =
                new GradientDrawable();

        fondo.setColor(PANEL_DARK);

        fondo.setCornerRadius(12);

        fondo.setStroke(
                1,
                Color.rgb(80, 96, 120)
        );

        campo.setBackground(fondo);
    }

    private void intentarLogin() {

        String usuario =
                usernameInput
                        .getText()
                        .toString()
                        .trim();

        String contraseña =
                passwordInput
                        .getText()
                        .toString();

        if (usuario.isEmpty()) {

            usernameInput.requestFocus();

            Toast.makeText(
                    this,
                    "Introduce el usuario.",
                    Toast.LENGTH_SHORT
            ).show();

            return;
        }

        if (contraseña.isEmpty()) {

            passwordInput.requestFocus();

            Toast.makeText(
                    this,
                    "Introduce la contraseña.",
                    Toast.LENGTH_SHORT
            ).show();

            return;
        }

        status.setText(
                "●  Datos recibidos\n" +
                "●  Protocolo WoW todavía no implementado"
        );

        status.setTextColor(GOLD);

        try {

            nativeLogin(
                    usuario,
                    contraseña
            );

        } catch (Exception ignored) {
        }
    }

    // ============================================================
    // PERSISTENCIA
    // ============================================================

    private void guardarCarpeta(
            Uri uri
    ) {

        getSharedPreferences(
                "wow_client",
                MODE_PRIVATE
        )
                .edit()
                .putString(
                        "wow_folder_uri",
                        uri.toString()
                )
                .apply();
    }

    private Uri obtenerCarpetaGuardada() {

        String value =
                getSharedPreferences(
                        "wow_client",
                        MODE_PRIVATE
                )
                        .getString(
                                "wow_folder_uri",
                                null
                        );

        if (value == null ||
                value.isEmpty()) {

            return null;
        }

        try {

            return Uri.parse(value);

        } catch (Exception e) {

            return null;
        }
    }

    // ============================================================
    // VFS JNI
    // ============================================================

    /*
     * Estas funciones son llamadas desde wow_engine.cpp.
     *
     * IMPORTANTE:
     *
     * C++ NO intenta abrir content:// directamente.
     * Java usa ContentResolver/DocumentFile y devuelve
     * los datos al motor nativo.
     */

    public boolean vfsExists(
            String relativePath
    ) {

        DocumentFile file =
                encontrarDocumento(
                        relativePath
                );

        return file != null;
    }

    public boolean vfsIsDirectory(
            String relativePath
    ) {

        DocumentFile file =
                encontrarDocumento(
                        relativePath
                );

        return file != null &&
                file.isDirectory();
    }

    public String[] vfsList(
            String relativePath
    ) {

        DocumentFile directory =
                encontrarDocumento(
                        relativePath
                );

        if (directory == null ||
                !directory.isDirectory()) {

            return new String[0];
        }

        DocumentFile[] files =
                directory.listFiles();

        List<String> result =
                new ArrayList<>();

        for (DocumentFile file : files) {

            String name =
                    file.getName();

            if (name != null) {

                if (file.isDirectory()) {

                    result.add(
                            name + "/"
                    );

                } else {

                    result.add(name);
                }
            }
        }

        return result.toArray(
                new String[0]
        );
    }

    public byte[] vfsReadFile(
            String relativePath
    ) {

        DocumentFile file =
                encontrarDocumento(
                        relativePath
                );

        if (file == null ||
                file.isDirectory()) {

            return null;
        }

        /*
         * Protección inicial.
         *
         * Para archivos grandes como MPQ/BLP/M2
         * posteriormente implementaremos streaming.
         *
         * Esta primera versión permite probar el VFS
         * sin cargar archivos gigantes accidentalmente.
         */

        final long MAX_READ =
                16L * 1024L * 1024L;

        if (file.length() > MAX_READ) {

            return null;
        }

        try {

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
                    new byte[64 * 1024];

            int count;

            long total = 0;

            while ((count = input.read(buffer)) != -1) {

                total += count;

                if (total > MAX_READ) {

                    input.close();

                    return null;
                }

                output.write(
                        buffer,
                        0,
                        count
                );
            }

            input.close();

            return output.toByteArray();

        } catch (Exception e) {

            return null;
        }
    }

    private DocumentFile encontrarDocumento(
            String relativePath
    ) {

        if (wowFolderUri == null ||
                relativePath == null) {

            return null;
        }

        DocumentFile current =
                DocumentFile.fromTreeUri(
                        this,
                        wowFolderUri
                );

        if (current == null) {
            return null;
        }

        String path =
                relativePath
                        .replace(
                                '\\',
                                '/'
                        );

        while (path.startsWith("/")) {

            path =
                    path.substring(1);
        }

        if (path.isEmpty()) {

            return current;
        }

        String[] parts =
                path.split("/");

        for (String part : parts) {

            if (part.isEmpty()) {
                continue;
            }

            DocumentFile next = null;

            for (DocumentFile child :
                    current.listFiles()) {

                String name =
                        child.getName();

                if (name != null &&
                        name.equalsIgnoreCase(part)) {

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
}
