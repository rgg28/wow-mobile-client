package com.wowmobile.client;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.content.Intent;
import android.net.Uri;
import android.graphics.Color;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("wowmobile");
    }

    private LinearLayout root;

    private TextView status;

    private EditText username;
    private EditText password;

    private Button connectButton;

    private LinearLayout characterPanel;

    private boolean modifierL1 = false;
    private boolean modifierR1 = false;

    private boolean worldControlsVisible = false;

    private float joystickX = 0.0f;
    private float joystickY = 0.0f;

    private int selectedCharacter = -1;

    private static final int PICK_WOW_FOLDER = 1001;

    private native void nativeInit();

    private native void nativeSetWowFolder(String path);

    private native void nativeLogin(
            String username,
            String password
    );

    private native void nativeSelectCharacter(
            int index
    );

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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setStatusBarColor(Color.BLACK);
        getWindow().setNavigationBarColor(Color.BLACK);

        nativeInit();

        mostrarSeleccionCarpeta();
    }

    private void mostrarSeleccionCarpeta() {

        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setPadding(60, 40, 60, 40);

        root.setBackgroundColor(Color.rgb(15, 15, 15));

        TextView title = new TextView(this);

        title.setText("WORLD OF WARCRAFT 3.3.5a");
        title.setTextColor(Color.WHITE);
        title.setTextSize(26);
        title.setGravity(Gravity.CENTER);

        root.addView(
                title,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        100
                )
        );

        TextView info = new TextView(this);

        info.setText(
                "Selecciona la carpeta donde tienes los datos extraídos del cliente."
        );

        info.setTextColor(Color.LTGRAY);
        info.setTextSize(16);
        info.setGravity(Gravity.CENTER);

        root.addView(
                info,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        100
                )
        );

        Button select = new Button(this);

        select.setText("SELECCIONAR CARPETA WOW");

        select.setOnClickListener(
                v -> abrirSelectorCarpeta()
        );

        root.addView(
                select,
                new LinearLayout.LayoutParams(
                        500,
                        80
                )
        );

        setContentView(root);
    }

    private void abrirSelectorCarpeta() {

        Intent intent =
                new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION |
                Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION |
                Intent.FLAG_GRANT_WRITE_URI_PERMISSION
        );

        startActivityForResult(
                intent,
                PICK_WOW_FOLDER
        );
    }

    @Override
    protected void onActivityResult(
            int requestCode,
            int resultCode,
            Intent data) {

        super.onActivityResult(
                requestCode,
                resultCode,
                data
        );

        if (
                requestCode == PICK_WOW_FOLDER &&
                resultCode == RESULT_OK &&
                data != null
        ) {

            Uri uri = data.getData();

            if (uri == null)
                return;

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

            mostrarLogin();
        }
    }

    private void mostrarLogin() {

        root = new LinearLayout(this);

        root.setOrientation(
                LinearLayout.VERTICAL
        );

        root.setGravity(
                Gravity.CENTER
        );

        root.setPadding(
                60,
                40,
                60,
                40
        );

        root.setBackgroundColor(
                Color.rgb(10, 10, 10)
        );

        TextView title = new TextView(this);

        title.setText(
                "WORLD OF WARCRAFT 3.3.5a"
        );

        title.setTextColor(
                Color.WHITE
        );

        title.setTextSize(
                26
        );

        title.setGravity(
                Gravity.CENTER
        );

        root.addView(
                title,
                new LinearLayout.LayoutParams(
                        500,
                        70
                )
        );

        username = new EditText(this);

        username.setHint(
                "Nombre de cuenta"
        );

        username.setSingleLine(true);

        root.addView(
                username,
                new LinearLayout.LayoutParams(
                        500,
                        70
                )
        );

        password = new EditText(this);

        password.setHint(
                "Contraseña"
        );

        password.setSingleLine(true);

        password.setInputType(
                0x00000081
        );

        root.addView(
                password,
                new LinearLayout.LayoutParams(
                        500,
                        70
                )
        );

        connectButton = new Button(this);

        connectButton.setText(
                "CONECTAR"
        );

        connectButton.setOnClickListener(
                v -> procesarLogin()
        );

        root.addView(
                connectButton,
                new LinearLayout.LayoutParams(
                        500,
                        80
                )
        );

        status = new TextView(this);

        status.setText(
                "Esperando conexión..."
        );

        status.setTextColor(
                Color.LTGRAY
        );

        status.setGravity(
                Gravity.CENTER
        );

        root.addView(
                status,
                new LinearLayout.LayoutParams(
                        500,
                        70
                )
        );

        setContentView(root);
    }

    private void procesarLogin() {

        String user =
                username.getText()
                        .toString()
                        .trim();

        String pass =
                password.getText()
                        .toString();

        if (user.isEmpty() ||
            pass.isEmpty()) {

            status.setText(
                    "Introduce usuario y contraseña."
            );

            return;
        }

        status.setText(
                "Conectando..."
        );

        connectButton.setEnabled(
                false
        );

        nativeLogin(
                user,
                pass
        );
    }

    public void mostrarPersonajes(
            String[] personajes
    ) {

        runOnUiThread(() -> {

            root = new LinearLayout(
                    MainActivity.this
            );

            root.setOrientation(
                    LinearLayout.VERTICAL
            );

            root.setGravity(
                    Gravity.CENTER
            );

            root.setPadding(
                    40,
                    30,
                    40,
                    30
            );

            root.setBackgroundColor(
                    Color.rgb(10, 10, 10)
            );

            TextView title =
                    new TextView(
                            MainActivity.this
                    );

            title.setText(
                    "SELECCIONA UN PERSONAJE"
            );

            title.setTextColor(
                    Color.WHITE
            );

            title.setTextSize(
                    22
            );

            title.setGravity(
                    Gravity.CENTER
            );

            root.addView(
                    title,
                    new LinearLayout.LayoutParams(
                            600,
                            70
                    )
            );

            for (int i = 0;
                 i < personajes.length;
                 i++) {

                final int index = i;

                Button character =
                        new Button(
                                MainActivity.this
                        );

                character.setText(
                        personajes[i]
                );

                character.setOnClickListener(
                        v -> {

                            selectedCharacter =
                                    index;

                            nativeSelectCharacter(
                                    index
                            );

                            mostrarControlesJuego();
                        }
                );

                root.addView(
                        character,
                        new LinearLayout.LayoutParams(
                                600,
                                75
                        )
                );
            }

            setContentView(root);
        });
    }

    private void mostrarControlesJuego() {

        worldControlsVisible = true;

        root = new LinearLayout(this);

        root.setOrientation(
                LinearLayout.VERTICAL
        );

        root.setBackgroundColor(
                Color.TRANSPARENT
        );

        status = new TextView(this);

        status.setText(
                "Entrando al mundo..."
        );

        status.setTextColor(
                Color.WHITE
        );

        status.setTextSize(
                18
        );

        status.setGravity(
                Gravity.CENTER
        );

        root.addView(
                status,
                new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        70
                )
        );

        LinearLayout modifiers =
                new LinearLayout(this);

        modifiers.setOrientation(
                LinearLayout.HORIZONTAL
        );

        Button l1 =
                crearBoton("L1");

        Button r1 =
                crearBoton("R1");

        l1.setOnTouchListener(
                (v, event) -> {

                    if (event.getAction() ==
                            MotionEvent.ACTION_DOWN) {

                        modifierL1 = true;
                        actualizarBotones();
                    }

                    if (event.getAction() ==
                            MotionEvent.ACTION_UP ||
                        event.getAction() ==
                            MotionEvent.ACTION_CANCEL) {

                        modifierL1 = false;
                        actualizarBotones();
                    }

                    return true;
                }
        );

        r1.setOnTouchListener(
                (v, event) -> {

                    if (event.getAction() ==
                            MotionEvent.ACTION_DOWN) {

                        modifierR1 = true;
                        actualizarBotones();
                    }

                    if (event.getAction() ==
                            MotionEvent.ACTION_UP ||
                        event.getAction() ==
                            MotionEvent.ACTION_CANCEL) {

                        modifierR1 = false;
                        actualizarBotones();
                    }

                    return true;
                }
        );

        modifiers.addView(
                l1,
                new LinearLayout.LayoutParams(
                        200,
                        80
                )
        );

        modifiers.addView(
                r1,
                new LinearLayout.LayoutParams(
                        200,
                        80
                )
        );

        root.addView(
                modifiers
        );

        LinearLayout buttons =
                new LinearLayout(this);

        buttons.setOrientation(
                LinearLayout.HORIZONTAL
        );

        Button x =
                crearBoton("Habilidad 1");

        Button y =
                crearBoton("Habilidad 2");

        Button b =
                crearBoton("Habilidad 3");

        Button a =
                crearBoton("Saltar");

        x.setOnClickListener(
                v -> ejecutarBoton(1)
        );

        y.setOnClickListener(
                v -> ejecutarBoton(2)
        );

        b.setOnClickListener(
                v -> ejecutarBoton(3)
        );

        a.setOnClickListener(
                v -> ejecutarBoton(4)
        );

        buttons.addView(
                x,
                new LinearLayout.LayoutParams(
                        180,
                        100
                )
        );

        buttons.addView(
                y,
                new LinearLayout.LayoutParams(
                        180,
                        100
                )
        );

        buttons.addView(
                b,
                new LinearLayout.LayoutParams(
                        180,
                        100
                )
        );

        buttons.addView(
                a,
                new LinearLayout.LayoutParams(
                        180,
                        100
                )
        );

        root.addView(
                buttons
        );

        setContentView(root);

        status.setText(
                "Controles preparados."
        );
    }

    private Button crearBoton(
            String texto
    ) {

        Button button =
                new Button(this);

        button.setText(
                texto
        );

        return button;
    }

    private void actualizarBotones() {

        if (status == null)
            return;

        if (modifierL1) {

            status.setText(
                    "Set L1 Activo"
            );

        } else if (modifierR1) {

            status.setText(
                    "Set R1 Activo"
            );

        } else {

            status.setText(
                    "Set Normal Activo"
            );
        }
    }

    private void ejecutarBoton(
            int boton
    ) {

        if (
                boton == 4 &&
                !modifierL1 &&
                !modifierR1
        ) {

            nativeJump();
            return;
        }

        int spell = 0;

        if (!modifierL1 &&
            !modifierR1) {

            if (boton == 1)
                spell = 47450;

            if (boton == 2)
                spell = 47471;

            if (boton == 3)
                spell = 47465;

        } else if (
                modifierL1 &&
                !modifierR1
        ) {

            if (boton == 1)
                spell = 11578;

            if (boton == 2)
                spell = 20252;

            if (boton == 3)
                spell = 48068;

            if (boton == 4)
                spell = 54729;

        } else if (
                !modifierL1 &&
                modifierR1
        ) {

            if (boton == 1)
                spell = 48156;

            if (boton == 2)
                spell = 48123;

            if (boton == 3)
                spell = 48160;

            if (boton == 4)
                spell = 33010;
        }

        if (spell != 0)
            nativeSpell(spell);
    }

    @Override
    public boolean onTouchEvent(
            MotionEvent event
    ) {

        if (worldControlsVisible) {

            nativeTouch(
                    event.getActionMasked(),
                    event.getX(),
                    event.getY()
            );
        }

        return true;
    }
}
