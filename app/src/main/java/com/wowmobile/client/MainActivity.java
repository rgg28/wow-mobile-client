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
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.documentfile.provider.DocumentFile;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Locale; // <--- CORREGIDO: Importación añadida

public class MainActivity extends Activity {

    private static final int REQUEST_WOW_FOLDER = 5001;

    private LinearLayout root;
    private Uri wowTreeUri;
    private DocumentFile wowRoot;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    static {
        System.loadLibrary("wowmobile");
    }

    // --- INTERFAZ JNI CON WOWEE ---
    private native void nativeInit();
    private native void initWoWEngine(Surface surface);
    private native void renderFrame();
    private native void connectToServer(String host, int port, String user, String pass);

    // --- MÉTODOS DE COLORES Y DISEÑO BASE ---
    private int bg() { return Color.rgb(5, 10, 20); }
    private int panel2() { return Color.rgb(15, 28, 48); }
    private int goldBright() { return Color.rgb(245, 200, 95); }
    private int textDim() { return Color.rgb(145, 158, 180); }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setNavigationBarColor(bg());
        getWindow().setStatusBarColor(bg());
        nativeInit();
        showFolderScreen();
    }

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
            e.setInputType(android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD);
        }
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(52));
        p.setMargins(0, dp(6), 0, dp(6));
        e.setLayoutParams(p);
        return e;
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }
    // ============================================================
    // FLUJO DE INTERFAZ, PARSEO DE REALMLIST.WTF Y JUEGO
    // ============================================================
    private void showFolderScreen() {
        root = createBase();
        root.addView(title("WORLD OF WARCRAFT", 27), new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(60)));
        
        TextView subtitle = title("MOBILE CLIENT", 15);
        subtitle.setTextColor(textDim());
        root.addView(subtitle);

        TextView info = label("Selecciona la carpeta raíz de tu cliente\nWoW 3.3.5a · Build 12340");
        info.setPadding(dp(20), dp(30), dp(20), dp(20));
        root.addView(info);

        Button select = button("SELECCIONAR CARPETA DEL CLIENTE");
        select.setOnClickListener(v -> openFolderPicker());
        root.addView(select, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(60)));

        setContentView(root);
    }

    private void openFolderPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_WOW_FOLDER);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_WOW_FOLDER && resultCode == Activity.RESULT_OK && data != null) {
            wowTreeUri = data.getData();
            getContentResolver().takePersistableUriPermission(wowTreeUri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
            wowRoot = DocumentFile.fromTreeUri(this, wowTreeUri);
            
            String parsedHost = readRealmlistWtf();
            showLoginScreen(parsedHost);
        }
    }

    private String readRealmlistWtf() {
        String host = "127.0.0.1";
        if (wowRoot == null) return host;

        DocumentFile realmlistFile = wowRoot.findFile("realmlist.wtf");
        if (realmlistFile == null) {
            DocumentFile dataDir = wowRoot.findFile("Data");
            if (dataDir != null) {
                for (DocumentFile subDir : dataDir.listFiles()) {
                    if (subDir.isDirectory()) {
                        realmlistFile = subDir.findFile("realmlist.wtf");
                        if (realmlistFile != null) break;
                    }
                }
            }
        }

        if (realmlistFile != null && realmlistFile.exists()) {
            try (InputStream is = getContentResolver().openInputStream(realmlistFile.getUri());
                 BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    line = line.trim();
                    if (line.toLowerCase(Locale.ROOT).startsWith("set realmlist")) {
                        String[] tokens = line.split("\\s+");
                        if (tokens.length >= 3) {
                            host = tokens[2];
                            break;
                        }
                    }
                }
            } catch (Exception ignored) {}
        }
        return host;
    }

    private void showLoginScreen(final String defaultHost) {
        LinearLayout loginLayout = createBase();
        loginLayout.addView(title("INICIAR SESIÓN", 24));
        
        final EditText editHost = edit("Servidor (Realmlist)", false);
        editHost.setText(defaultHost);
        final EditText editUser = edit("Nombre de Cuenta", false);
        final EditText editPass = edit("Contraseña", true);

        loginLayout.addView(editHost);
        loginLayout.addView(editUser);
        loginLayout.addView(editPass);

        Button btnConnect = button("CONECTAR");
        btnConnect.setOnClickListener(v -> {
            String host = editHost.getText().toString().trim();
            String user = editUser.getText().toString().trim();
            String pass = editPass.getText().toString().trim();

            if(!host.isEmpty() && !user.isEmpty() && !pass.isEmpty()) {
                showGameScreen(host, user, pass);
            }
        });
        loginLayout.addView(btnConnect, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(55)));
        setContentView(loginLayout);
    }

    private void showGameScreen(final String host, final String user, final String pass) {
        SurfaceView surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                // CORREGIDO: Usando holder.getSurface() nativo de Android
                initWoWEngine(holder.getSurface()); 
                new Thread(() -> connectToServer(host, 3724, user, pass)).start();
                new Thread(() -> {
                    while (!isFinishing()) {
                        renderFrame();
                        try { Thread.sleep(16); } catch (InterruptedException ignored) {}
                    }
                }).start();
            }
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {}
        });
        setContentView(surfaceView);
    }

    public String vfsList(String path) {
        if (wowRoot == null) return "";
        DocumentFile target = wowRoot;
        if (!path.isEmpty() && !path.equals("/")) {
            String[] parts = path.split("/");
            for (String part : parts) {
                if (!part.isEmpty()) {
                    target = target.findFile(part);
                    if (target == null) return "";
                }
            }
        }
        StringBuilder sb = new StringBuilder();
        if (target.isDirectory()) {
            for (DocumentFile file : target.listFiles()) {
                sb.append(file.getName()).append(file.isDirectory() ? "/" : "").append("\n");
            }
        }
        return sb.toString();
    }
}
