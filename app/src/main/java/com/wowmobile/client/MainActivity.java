package com.wowmobile.client;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.graphics.Color;
import android.graphics.Typeface;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
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

    private static final int REQUEST_WOW_FOLDER = 5001;

    private LinearLayout root;
    private LinearLayout content;

    private Uri wowTreeUri;
    private DocumentFile wowRoot;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    static {
        System.loadLibrary("wowmobile");
    }

    // ------------------------------------------------------------
    // JNI
    // ------------------------------------------------------------

    private native void nativeInit();
    private native void nativeSetWowFolder(String treeUri);
    private native void nativeInspectRoot();

    private native void nativeRendererSetBackend(int backend);
    private native void nativeRendererSetSurface(Surface surface);
    private native void nativeRendererResize(int width, int height);
    private native void nativeRendererStop();

    private native void nativeRendererCamera(float x, float y);
    private native void nativeRendererZoom(float delta);

    private native int nativeRendererGetBackend();

    private native boolean nativeRendererLoadCharacter(
            byte[] m2Data,
            byte[] skinData
    );

    private native boolean nativeRendererLoadWorld(
            byte[] adtData
    );

    // ------------------------------------------------------------
    // COLORES
    // ------------------------------------------------------------

    private int bg() {
        return Color.rgb(5, 10, 20);
    }

    private int panel() {
        return Color.rgb(11, 20, 35);
    }

    private int panel2() {
        return Color.rgb(15, 28, 48);
    }

    private int gold() {
        return Color.rgb(218, 168, 72);
    }

    private int goldBright() {
        return Color.rgb(245, 200, 95);
    }

    private int text() {
        return Color.rgb(225, 231, 240);
    }

    private int textDim() {
        return Color.rgb(145, 158, 180);
    }

    // ------------------------------------------------------------
    // ACTIVITY
    // ------------------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setNavigationBarColor(bg());
        getWindow().setStatusBarColor(bg());

        nativeInit();

        showFolderScreen();
    }

    @Override
    protected void onDestroy() {
        try {
            nativeRendererStop();
        } catch (Exception ignored) {
        }

        super.onDestroy();
    }

    // ------------------------------------------------------------
    // BASE UI
    // ------------------------------------------------------------

    private LinearLayout createBase() {

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(dp(28), dp(20), dp(28), dp(20));
        layout.setBackgroundColor(bg());

        return layout;
    }

    private TextView title(String value, float size) {

        TextView v = new TextView(this);

        v.setText(value);
        v.setTextColor(goldBright());
        v.setTextSize(size);
        v.setGravity(Gravity.CENTER);
        v.setTypeface(Typeface.create("sans-serif", Typeface.BOLD));
        v.setPadding(0, dp(6), 0, dp(6));

        return v;
    }

    private TextView label(String value) {

        TextView v = new TextView(this);

        v.setText(value);
        v.setTextColor(textDim());
        v.setTextSize(15);
        v.setGravity(Gravity.CENTER);
        v.setPadding(dp(8), dp(8), dp(8), dp(8));

        return v;
    }

    private Button button(String value) {

        Button b = new Button(this);

        b.setText(value);
        b.setTextColor(Color.WHITE);
        b.setTextSize(15);
        b.setAllCaps(false);
        b.setBackgroundColor(panel2());
        b.setPadding(dp(20), dp(8), dp(20), dp(8));

        return b;
    }

    private EditText edit(String hint, boolean password) {

        EditText e = new EditText(this);

        e.setHint(hint);
        e.setHintTextColor(textDim());
        e.setTextColor(Color.WHITE);
        e.setTextSize(16);
        e.setSingleLine(true);
        e.setPadding(dp(16), 0, dp(16), 0);
        e.setBackgroundColor(panel2());

        if (password) {
            e.setInputType(
                    android.text.InputType.TYPE_CLASS_TEXT |
                    android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD
            );
        }

        LinearLayout.LayoutParams p =
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(52)
                );

        p.setMargins(0, dp(6), 0, dp(6));
        e.setLayoutParams(p);

        return e;
    }

    private void setScreen(View view) {
        setContentView(view);
    }

    private int dp(int value) {
        return (int) (
                value * getResources().getDisplayMetrics().density + 0.5f
        );
    }

    // ------------------------------------------------------------
    // 1. SELECCION CLIENTE
    // ------------------------------------------------------------

    private void showFolderScreen() {

        root = createBase();

        TextView logo = title("WORLD OF WARCRAFT", 27);
        root.addView(
                logo,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(60)
                )
        );

        TextView subtitle = title("MOBILE CLIENT", 15);
        subtitle.setTextColor(textDim());

        root.addView(subtitle);

        TextView info = label(
                "Selecciona la carpeta raíz de tu cliente\n" +
                "WoW 3.3.5a · Build 12340"
        );

        info.setPadding(
                dp(20),
                dp(30),
                dp(20),
                dp(20)
        );

        root.addView(info);

        Button select = button("SELECCIONAR CARPETA DEL CLIENTE");

        select.setOnClickListener(v -> openFolderPicker());

        LinearLayout.LayoutParams bp =
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(60)
                );

        bp.setMargins(0, dp(20), 0, dp(8));

        root.addView(select, bp);

        TextView note = label(
                "Los archivos permanecen fuera del APK.\n" +
                "El cliente los lee directamente desde la carpeta seleccionada."
        );

        note.setTextSize(12);

        root.addView(note);

        setScreen(root);
    }

    private void openFolderPicker() {

        Intent intent =
                new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);

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

        if (requestCode != REQUEST_WOW_FOLDER ||
                resultCode != RESULT_OK ||
                data == null ||
                data.getData() == null) {

            return;
        }

        wowTreeUri = data.getData();

        try {

            getContentResolver().takePersistableUriPermission(
                    wowTreeUri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION
            );

        } catch (Exception ignored) {
        }

        wowRoot =
                DocumentFile.fromTreeUri(
                        this,
                        wowTreeUri
                );

        if (wowRoot == null || !wowRoot.isDirectory()) {

            Toast.makeText(
                    this,
                    "No se pudo abrir la carpeta.",
                    Toast.LENGTH_LONG
            ).show();

            return;
        }

        nativeSetWowFolder(
                wowTreeUri.toString()
        );

        showOfflineMenu();
    }

    // ------------------------------------------------------------
    // 2. MENU PRINCIPAL
    // ------------------------------------------------------------

    private void showOfflineMenu() {

        root = createBase();

        root.addView(title("WORLD OF WARCRAFT", 25));

        root.addView(
                label(
                        "CLIENTE DETECTADO\n" +
                        "Build 12340"
                )
        );

        Button login =
                button("INICIAR SESIÓN");

        login.setOnClickListener(
                v -> showLogin()
        );

        root.addView(
                login,
                fullButtonParams()
        );

        Button characters =
                button("SELECCIÓN DE PERSONAJE");

        characters.setOnClickListener(
                v -> showCharacterSelection()
        );

        root.addView(
                characters,
                fullButtonParams()
        );

        Button create =
                button("CREAR / VISUALIZAR PERSONAJE");

        create.setOnClickListener(
                v -> showCharacterCreator()
        );

        root.addView(
                create,
                fullButtonParams()
        );

        Button world =
                button("ENTRAR AL MUNDO");

        world.setOnClickListener(
                v -> findAndEnterWorld()
        );

        root.addView(
                world,
                fullButtonParams()
        );

        Button inspect =
                button("INSPECCIONAR CLIENTE");

        inspect.setOnClickListener(
                v -> {

                    nativeInspectRoot();

                    Toast.makeText(
                            this,
                            "Cliente enviado al motor nativo.",
                            Toast.LENGTH_SHORT
                    ).show();
                }
        );

        root.addView(
                inspect,
                fullButtonParams()
        );

        setScreen(root);
    }

    private LinearLayout.LayoutParams fullButtonParams() {

        LinearLayout.LayoutParams p =
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(58)
                );

        p.setMargins(
                dp(20),
                dp(7),
                dp(20),
                dp(7)
        );

        return p;
    }

    // ------------------------------------------------------------
    // 3. LOGIN
    // ------------------------------------------------------------

    private void showLogin() {

        root = createBase();

        root.addView(
                title("INICIAR SESIÓN", 24)
        );

        root.addView(
                label(
                        "Conexión WoW 3.3.5a"
                )
        );

        EditText username =
                edit("Nombre de usuario", false);

        EditText password =
                edit("Contraseña", true);

        root.addView(username);
        root.addView(password);

        Button connect =
                button("CONECTAR");

        connect.setOnClickListener(v -> {

            String user =
                    username.getText()
                            .toString()
                            .trim();

            String pass =
                    password.getText()
                            .toString();

            if (user.isEmpty() || pass.isEmpty()) {

                Toast.makeText(
                        this,
                        "Introduce usuario y contraseña.",
                        Toast.LENGTH_SHORT
                ).show();

                return;
            }

            Toast.makeText(
                    this,
                    "La interfaz está preparada para AUTH SRP6.",
                    Toast.LENGTH_LONG
            ).show();

            /*
             * Aquí se conectará posteriormente:
             *
             * AUTH_LOGON_CHALLENGE
             * AUTH_LOGON_PROOF
             * REALM_LIST
             *
             * No simulamos una conexión real.
             */

        });

        root.addView(
                connect,
                fullButtonParams()
        );

        Button back =
                button("VOLVER");

        back.setOnClickListener(
                v -> showOfflineMenu()
        );

        root.addView(
                back,
                fullButtonParams()
        );

        setScreen(root);
    }

    // ------------------------------------------------------------
    // 4. SELECCION PERSONAJE
    // ------------------------------------------------------------

    private void showCharacterSelection() {

        root = createBase();

        root.addView(
                title("SELECCIÓN DE PERSONAJE", 23)
        );

        TextView charInfo =
                label(
                        "ARTHAS\n" +
                        "Nivel 80 · Paladín\n" +
                        "Personaje de prueba"
                );

        charInfo.setTextSize(18);
        charInfo.setTextColor(text());

        root.addView(
                charInfo
        );

        Button enter =
                button("ENTRAR AL MUNDO");

        enter.setOnClickListener(
                v -> findAndEnterWorld()
        );

        root.addView(
                enter,
                fullButtonParams()
        );

        Button create =
                button("CREAR / VISUALIZAR");

        create.setOnClickListener(
                v -> showCharacterCreator()
        );

        root.addView(
                create,
                fullButtonParams()
        );

        Button back =
                button("VOLVER");

        back.setOnClickListener(
                v -> showOfflineMenu()
        );

        root.addView(
                back,
                fullButtonParams()
        );

        setScreen(root);
    }

    // ------------------------------------------------------------
    // 5. RENDER PERSONAJE
    // ------------------------------------------------------------

    private void showCharacterCreator() {

        final SurfaceView surface =
                new SurfaceView(this);

        final LinearLayout container =
                new LinearLayout(this);

        container.setOrientation(
                LinearLayout.VERTICAL
        );

        container.setBackgroundColor(bg());

        TextView header =
                title(
                        "PERSONAJE · RENDER M2/SKIN",
                        17
                );

        container.addView(
                header,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(48)
                )
        );

        container.addView(
                surface,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        0,
                        1
                )
        );

        Button back =
                button("VOLVER");

        back.setOnClickListener(v -> {

            nativeRendererStop();

            showCharacterSelection();
        });

        container.addView(
                back,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(54)
                )
        );

        setScreen(container);

        surface.getHolder().addCallback(
                new SurfaceHolder.Callback() {

                    @Override
                    public void surfaceCreated(
                            SurfaceHolder holder
                    ) {

                        nativeRendererSetBackend(1);

                        nativeRendererSetSurface(
                                holder.getSurface()
                        );

                        loadCharacterAsync();
                    }

                    @Override
                    public void surfaceChanged(
                            SurfaceHolder holder,
                            int format,
                            int width,
                            int height
                    ) {

                        nativeRendererResize(
                                width,
                                height
                        );
                    }

                    @Override
                    public void surfaceDestroyed(
                            SurfaceHolder holder
                    ) {

                        nativeRendererStop();
                    }
                }
        );

        surface.setOnTouchListener(
                (v, event) -> {

                    if (event.getAction() ==
                            MotionEvent.ACTION_MOVE) {

                        nativeRendererCamera(
                                event.getX(),
                                event.getY()
                        );

                    }

                    if (event.getAction() ==
                            MotionEvent.ACTION_DOWN) {

                        return true;
                    }

                    return true;
                }
        );
    }

    private void loadCharacterAsync() {

        new Thread(() -> {

            try {

                DocumentFile model =
                        findFirstFile(
                                wowRoot,
                                ".m2",
                                5,
                                150
                        );

                if (model == null) {

                    runOnUiThread(() ->
                            Toast.makeText(
                                    this,
                                    "No se encontró ningún M2.",
                                    Toast.LENGTH_LONG
                            ).show()
                    );

                    return;
                }

                byte[] m2 =
                        readFile(model, 32 * 1024 * 1024);

                DocumentFile skin =
                        findSkin(
                                wowRoot,
                                model
                        );

                byte[] skinData =
                        skin != null
                                ? readFile(
                                        skin,
                                        8 * 1024 * 1024
                                )
                                : new byte[0];

                boolean ok =
                        nativeRendererLoadCharacter(
                                m2,
                                skinData
                        );

                runOnUiThread(() -> {

                    Toast.makeText(
                            this,
                            ok
                                    ? "Modelo M2 cargado."
                                    : "No se pudo cargar el modelo.",
                            Toast.LENGTH_SHORT
                    ).show();

                });

            } catch (Exception e) {

                runOnUiThread(() ->
                        Toast.makeText(
                                this,
                                "Error leyendo M2: " +
                                        e.getMessage(),
                                Toast.LENGTH_LONG
                        ).show()
                );
            }

        }).start();
    }

    // ------------------------------------------------------------
    // 6. ENTRAR AL MUNDO
    // ------------------------------------------------------------

    private void findAndEnterWorld() {

        if (wowRoot == null) {

            Toast.makeText(
                    this,
                    "Primero selecciona el cliente.",
                    Toast.LENGTH_LONG
            ).show();

            return;
        }

        Toast.makeText(
                this,
                "Buscando terreno ADT...",
                Toast.LENGTH_SHORT
        ).show();

        new Thread(() -> {

            try {

                /*
                 * No hacemos un escaneo completo del cliente.
                 *
                 * Buscamos solamente un ADT hasta profundidad 6
                 * y un máximo de 80 candidatos.
                 */

                DocumentFile adt =
                        findFirstFile(
                                wowRoot,
                                ".adt",
                                6,
                                80
                        );

                if (adt == null) {

                    runOnUiThread(() ->
                            Toast.makeText(
                                    this,
                                    "No se encontró ningún archivo ADT.",
                                    Toast.LENGTH_LONG
                            ).show()
                    );

                    return;
                }

                byte[] data =
                        readFile(
                                adt,
                                32 * 1024 * 1024
                        );

                boolean valid =
                        data.length >= 8 &&
                        containsChunk(
                                data,
                                "MCNK"
                        );

                if (!valid) {

                    runOnUiThread(() ->
                            Toast.makeText(
                                    this,
                                    "El ADT encontrado no contiene MCNK válido.",
                                    Toast.LENGTH_LONG
                            ).show()
                    );

                    return;
                }

                runOnUiThread(() ->
                        showWorldScreen(
                                adt.getName(),
                                data
                        )
                );

            } catch (Exception e) {

                runOnUiThread(() ->
                        Toast.makeText(
                                this,
                                "Error cargando mundo: " +
                                        e.getMessage(),
                                Toast.LENGTH_LONG
                        ).show()
                );
            }

        }).start();
    }

    // ------------------------------------------------------------
    // 7. WORLD VIEW
    // ------------------------------------------------------------

    private void showWorldScreen(
            String fileName,
            byte[] adtData
    ) {

        final LinearLayout container =
                new LinearLayout(this);

        container.setOrientation(
                LinearLayout.VERTICAL
        );

        container.setBackgroundColor(bg());

        TextView header =
                title(
                        "WORLD · " +
                                (fileName != null
                                        ? fileName
                                        : "ADT"),
                        15
                );

        container.addView(
                header,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(42)
                )
        );

        final SurfaceView surface =
                new SurfaceView(this);

        container.addView(
                surface,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        0,
                        1
                )
        );

        LinearLayout controls =
                createWorldControls();

        container.addView(
                controls,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(90)
                )
        );

        Button exit =
                button("SALIR DEL MUNDO");

        exit.setOnClickListener(v -> {

            nativeRendererStop();

            showCharacterSelection();
        });

        container.addView(
                exit,
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        dp(48)
                )
        );

        setScreen(container);

        surface.getHolder().addCallback(
                new SurfaceHolder.Callback() {

                    @Override
                    public void surfaceCreated(
                            SurfaceHolder holder
                    ) {

                        nativeRendererSetBackend(1);

                        nativeRendererSetSurface(
                                holder.getSurface()
                        );

                        /*
                         * Esperamos a que el renderer tenga
                         * superficie y luego entregamos el ADT.
                         */

                        mainHandler.postDelayed(
                                () -> {

                                    nativeRendererLoadWorld(
                                            adtData
                                    );

                                },
                                150
                        );
                    }

                    @Override
                    public void surfaceChanged(
                            SurfaceHolder holder,
                            int format,
                            int width,
                            int height
                    ) {

                        nativeRendererResize(
                                width,
                                height
                        );
                    }

                    @Override
                    public void surfaceDestroyed(
                            SurfaceHolder holder
                    ) {

                        nativeRendererStop();
                    }
                }
        );

        surface.setOnTouchListener(
                (v, event) -> {

                    if (event.getAction() ==
                            MotionEvent.ACTION_MOVE) {

                        nativeRendererCamera(
                                event.getX(),
                                event.getY()
                        );

                    }

                    return true;
                }
        );
    }

    private LinearLayout createWorldControls() {

        LinearLayout controls =
                new LinearLayout(this);

        controls.setOrientation(
                LinearLayout.HORIZONTAL
        );

        controls.setGravity(
                Gravity.CENTER
        );

        controls.setBackgroundColor(panel());

        Button left =
                button("◀");

        Button zoomIn =
                button("+");

        Button zoomOut =
                button("−");

        Button right =
                button("▶");

        View.OnClickListener empty =
                v -> {
                };

        left.setOnClickListener(
                v -> nativeRendererCamera(-30, 0)
        );

        right.setOnClickListener(
                v -> nativeRendererCamera(30, 0)
        );

        zoomIn.setOnClickListener(
                v -> nativeRendererZoom(-1)
        );

        zoomOut.setOnClickListener(
                v -> nativeRendererZoom(1)
        );

        controls.addView(left, controlParams());
        controls.addView(zoomIn, controlParams());
        controls.addView(zoomOut, controlParams());
        controls.addView(right, controlParams());

        return controls;
    }

    private LinearLayout.LayoutParams controlParams() {

        LinearLayout.LayoutParams p =
                new LinearLayout.LayoutParams(
                        dp(70),
                        dp(58)
                );

        p.setMargins(
                dp(6),
                dp(8),
                dp(6),
                dp(8)
        );

        return p;
    }

    // ------------------------------------------------------------
    // FILE SEARCH
    // ------------------------------------------------------------

    private DocumentFile findFirstFile(
            DocumentFile dir,
            String extension,
            int depth,
            int maxCandidates
    ) {

        if (dir == null ||
                !dir.isDirectory() ||
                depth < 0) {

            return null;
        }

        DocumentFile[] children =
                dir.listFiles();

        int inspected = 0;

        for (DocumentFile f : children) {

            if (inspected++ >= maxCandidates) {
                return null;
            }

            if (f.isFile()) {

                String name =
                        f.getName();

                if (name != null &&
                        name.toLowerCase(
                                Locale.ROOT
                        ).endsWith(
                                extension.toLowerCase(
                                        Locale.ROOT
                                )
                        )) {

                    return f;
                }
            }
        }

        for (DocumentFile f : children) {

            if (f.isDirectory()) {

                DocumentFile result =
                        findFirstFile(
                                f,
                                extension,
                                depth - 1,
                                maxCandidates
                        );

                if (result != null) {
                    return result;
                }
            }
        }

        return null;
    }

    private DocumentFile findSkin(
            DocumentFile root,
            DocumentFile model
    ) {

        String modelName =
                model.getName();

        if (modelName == null) {
            return null;
        }

        if (modelName.toLowerCase(
                Locale.ROOT
        ).endsWith(".m2")) {

            String base =
                    modelName.substring(
                            0,
                            modelName.length() - 3
                    );

            String wanted =
                    base + "skin";

            return findNamedFile(
                    root,
                    wanted,
                    6
            );
        }

        return null;
    }

    private DocumentFile findNamedFile(
            DocumentFile dir,
            String wanted,
            int depth
    ) {

        if (dir == null ||
                !dir.isDirectory() ||
                depth < 0) {

            return null;
        }

        DocumentFile[] children =
                dir.listFiles();

        for (DocumentFile f : children) {

            String name =
                    f.getName();

            if (f.isFile() &&
                    name != null &&
                    name.equalsIgnoreCase(wanted)) {

                return f;
            }
        }

        for (DocumentFile f : children) {

            if (f.isDirectory()) {

                DocumentFile result =
                        findNamedFile(
                                f,
                                wanted,
                                depth - 1
                        );

                if (result != null) {
                    return result;
                }
            }
        }

        return null;
    }

    private byte[] readFile(
            DocumentFile file,
            int maxSize
    ) throws Exception {

        long length =
                file.length();

        if (length > maxSize) {
            throw new Exception(
                    "Archivo demasiado grande: " +
                            length
            );
        }

        InputStream input =
                getContentResolver()
                        .openInputStream(
                                file.getUri()
                        );

        if (input == null) {
            throw new Exception(
                    "No se pudo abrir el archivo."
            );
        }

        ByteArrayOutputStream out =
                new ByteArrayOutputStream();

        byte[] buffer =
                new byte[64 * 1024];

        int read;

        while ((read =
                input.read(buffer)) != -1) {

            out.write(
                    buffer,
                    0,
                    read
            );

            if (out.size() > maxSize) {

                input.close();

                throw new Exception(
                        "Archivo supera el límite."
                );
            }
        }

        input.close();

        return out.toByteArray();
    }

    private boolean containsChunk(
            byte[] data,
            String chunk
    ) {

        if (chunk.length() != 4) {
            return false;
        }

        byte[] tag =
                chunk.getBytes(
                        java.nio.charset.StandardCharsets.US_ASCII
                );

        for (int i = 0;
             i + 4 <= data.length;
             i++) {

            if (data[i] == tag[0] &&
                    data[i + 1] == tag[1] &&
                    data[i + 2] == tag[2] &&
                    data[i + 3] == tag[3]) {

                return true;
            }
        }

        return false;
    }
}
