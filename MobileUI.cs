using Godot;
using System.Collections.Generic;

public partial class MobileUI : CanvasLayer
{
    private bool _modL1Activo = false;
    private bool _modR1Activo = false;

    private NetworkClient? _network;

    private Label? _labelStatus;

    private Button? _btnA;
    private Button? _btnB;
    private Button? _btnX;
    private Button? _btnY;

    private Control? _joystickBase;
    private Button? _joystickPivote;

    private bool _joystickTocado = false;
    private Vector2 _joystickVector = Vector2.Zero;

    private float _joystickRadioMax = 50f;

    private MobileUIRadialMenu? _menuRadial;
    private bool _menuRadialActivo = false;

    private Control? _contenedorControlesJuego;

    private MobileUILogin? _pantallaLogin;
    private MobileUICharSelect? _pantallaCharSelect;

    public override void _Ready()
    {
        GD.Print("[UI] MobileUI iniciado.");

        _network =
            GetNodeOrNull<NetworkClient>(
                "/root/NetworkClient"
            );

        if (_network == null)
        {
            GD.PrintErr(
                "[UI] NetworkClient no encontrado."
            );
        }

        Control control = new Control();

        control.SetAnchorsPreset(
            Control.LayoutPreset.FullRect
        );

        AddChild(control);

        _contenedorControlesJuego =
            new Control();

        _contenedorControlesJuego.SetAnchorsPreset(
            Control.LayoutPreset.FullRect
        );

        _contenedorControlesJuego.Visible = false;

        control.AddChild(
            _contenedorControlesJuego
        );

        CrearEstado();

        CrearModificadores();

        CrearJoystick();

        CrearBotones();

        CrearMenuRadial();

        CrearSeleccionPersonaje(
            control
        );

        CrearLogin(
            control
        );

        ActualizarBotones();

        GD.Print("[UI] MobileUI completamente inicializado.");
    }

    private void CrearEstado()
    {
        if (_contenedorControlesJuego == null)
            return;

        _labelStatus = new Label
        {
            Text = "Azeroth te espera...",
            HorizontalAlignment =
                HorizontalAlignment.Center
        };

        _labelStatus.SetAnchorsPreset(
            Control.LayoutPreset.CenterTop
        );

        _labelStatus.Position =
            new Vector2(-200, 40);

        _labelStatus.Size =
            new Vector2(400, 50);

        _contenedorControlesJuego.AddChild(
            _labelStatus
        );
    }

    private void CrearModificadores()
    {
        if (_contenedorControlesJuego == null)
            return;

        Button btnL1 = new Button
        {
            Text = "MOD (L1)"
        };

        btnL1.Position =
            new Vector2(60, 40);

        btnL1.CustomMinimumSize =
            new Vector2(140, 70);

        btnL1.ButtonDown += () =>
        {
            _modL1Activo = true;
            ActualizarBotones();
        };

        btnL1.ButtonUp += () =>
        {
            _modL1Activo = false;
            ActualizarBotones();
        };

        _contenedorControlesJuego.AddChild(
            btnL1
        );

        Button btnR1 = new Button
        {
            Text = "MOD (R1)"
        };

        btnR1.Position =
            new Vector2(1080, 40);

        btnR1.CustomMinimumSize =
            new Vector2(140, 70);

        btnR1.ButtonDown += () =>
        {
            _modR1Activo = true;
            ActualizarBotones();
        };

        btnR1.ButtonUp += () =>
        {
            _modR1Activo = false;
            ActualizarBotones();
        };

        _contenedorControlesJuego.AddChild(
            btnR1
        );
    }

    private void CrearJoystick()
    {
        if (_contenedorControlesJuego == null)
            return;

        _joystickBase = new Control
        {
            CustomMinimumSize =
                new Vector2(120, 120)
        };

        _joystickBase.Position =
            new Vector2(100, 500);

        _contenedorControlesJuego.AddChild(
            _joystickBase
        );

        Panel panel =
            new Panel
            {
                CustomMinimumSize =
                    new Vector2(120, 120)
            };

        _joystickBase.AddChild(
            panel
        );

        _joystickPivote =
            new Button
            {
                CustomMinimumSize =
                    new Vector2(50, 50)
            };

        _joystickPivote.Position =
            new Vector2(35, 35);

        _joystickPivote.GuiInput +=
            OnJoystickGuiInput;

        _joystickBase.AddChild(
            _joystickPivote
        );
    }

    private void CrearBotones()
    {
        if (_contenedorControlesJuego == null)
            return;

        Control panelBotones =
            new Control();

        panelBotones.Position =
            new Vector2(1000, 450);

        _contenedorControlesJuego.AddChild(
            panelBotones
        );

        _btnX = CrearBoton(
            "Habilidad 1",
            new Vector2(0, 70),
            () => EjecutarJugabilidad(1)
        );

        _btnY = CrearBoton(
            "Habilidad 2",
            new Vector2(80, 0),
            () => EjecutarJugabilidad(2)
        );

        _btnB = CrearBoton(
            "Habilidad 3",
            new Vector2(160, 70),
            () => EjecutarJugabilidad(3)
        );

        _btnA = CrearBoton(
            "Saltar",
            new Vector2(80, 140),
            () => EjecutarJugabilidad(4)
        );

        panelBotones.AddChild(_btnX);
        panelBotones.AddChild(_btnY);
        panelBotones.AddChild(_btnB);
        panelBotones.AddChild(_btnA);
    }

    private Button CrearBoton(
        string texto,
        Vector2 posicion,
        System.Action accion)
    {
        Button boton = new Button
        {
            Text = texto,
            Position = posicion,
            CustomMinimumSize =
                new Vector2(80, 80),
            FocusMode =
                Control.FocusModeEnum.None
        };

        boton.Pressed += accion;

        return boton;
    }

    private void CrearMenuRadial()
    {
        if (_contenedorControlesJuego == null)
            return;

        _menuRadial =
            new MobileUIRadialMenu();

        _contenedorControlesJuego.AddChild(
            _menuRadial
        );
    }

    private void CrearSeleccionPersonaje(
        Control parent)
    {
        _pantallaCharSelect =
            new MobileUICharSelect();

        _pantallaCharSelect.Inicializar(
            AlSeleccionarPersonajeFinal
        );

        parent.AddChild(
            _pantallaCharSelect
        );
    }

    private void CrearLogin(
        Control parent)
    {
        _pantallaLogin =
            new MobileUILogin();

        _pantallaLogin.Inicializar(
            AlProcesarLogin
        );

        parent.AddChild(
            _pantallaLogin
        );
    }

    private void AlProcesarLogin(
        string usuario,
        string contrasena)
    {
        GD.Print(
            $"[UI] Login solicitado: {usuario}"
        );

        if (_network != null)
        {
            _network.IniciarSesion(
                usuario,
                contrasena
            );
        }

        List<string> personajesSimulados =
            new List<string>
            {
                "Arthas - Nivel 80 Paladín",
                "Thrall - Nivel 80 Chamán",
                "Malfurion - Nivel 74 Druida"
            };

        _pantallaCharSelect?.MostrarPersonajes(
            personajesSimulados
        );
    }

    private void AlSeleccionarPersonajeFinal(
        string nombrePersonaje)
    {
        string nombre =
            nombrePersonaje.Split(' ')[0];

        _network?.EnviarComandoMovimiento(
            $"LOGIN_WITH_{nombre.ToUpperInvariant()}"
        );

        if (_contenedorControlesJuego != null)
        {
            _contenedorControlesJuego.Visible =
                true;
        }

        if (_labelStatus != null)
        {
            _labelStatus.Text =
                $"Jugando como: {nombre}";
        }
    }

    public override void _Input(
        InputEvent @event)
    {
        if (_contenedorControlesJuego == null ||
            !_contenedorControlesJuego.Visible)
            return;

        if (@event is InputEventScreenTouch touch)
        {
            float ancho =
                GetViewport()
                    .GetVisibleRect()
                    .Size.X;

            float tercio =
                ancho / 3f;

            if (
                touch.Pressed &&
                touch.Position.X > tercio &&
                touch.Position.X < tercio * 2 &&
                !_joystickTocado
            )
            {
                _menuRadialActivo = true;

                _menuRadial?.Mostrar(
                    touch.Position
                );

                if (_labelStatus != null)
                    _labelStatus.Text =
                        "Desliza para elegir...";
            }
            else if (
                !touch.Pressed &&
                _menuRadialActivo
            )
            {
                _menuRadialActivo = false;

                int seleccion =
                    _menuRadial?.OcultarYSoltar()
                    ?? -1;

                if (seleccion != -1)
                {
                    _network?.EnviarComandoInterfaz(
                        seleccion switch
                        {
                            0 => "TOGGLE_CHARACTER_SHEET",
                            1 => "TOGGLE_BAGS",
                            2 => "TOGGLE_SPELLBOOK",
                            3 => "TOGGLE_LFG_PARENT",
                            _ => "TOGGLE_BATTLEGROUND"
                        }
                    );
                }

                ActualizarBotones();
            }
        }
        else if (
            @event is InputEventScreenDrag drag &&
            _menuRadialActivo
        )
        {
            _menuRadial?.ActualizarArrastre(
                drag.Position,
                _labelStatus
            );
        }
    }

    private void OnJoystickGuiInput(
        InputEvent @event)
    {
        if (_joystickBase == null ||
            _joystickPivote == null)
            return;

        if (@event is InputEventScreenTouch touch)
        {
            _joystickTocado =
                touch.Pressed;

            if (!_joystickTocado)
            {
                _joystickVector =
                    Vector2.Zero;

                _joystickPivote.Position =
                    new Vector2(35, 35);
            }
        }
        else if (
            @event is InputEventScreenDrag &&
            _joystickTocado
        )
        {
            Vector2 centro =
                new Vector2(60, 60);

            Vector2 vec =
                _joystickBase.GetLocalMousePosition()
                - centro;

            if (vec.Length() >
                _joystickRadioMax)
            {
                vec =
                    vec.Normalized() *
                    _joystickRadioMax;
            }

            _joystickVector =
                vec / _joystickRadioMax;

            _joystickPivote.Position =
                (centro + vec) -
                new Vector2(25, 25);
        }
    }

    public override void _PhysicsProcess(
        double delta)
    {
        if (
            _joystickVector != Vector2.Zero &&
            !_menuRadialActivo &&
            _contenedorControlesJuego != null &&
            _contenedorControlesJuego.Visible
        )
        {
            GD.Print(
                $"[Movimiento] X={_joystickVector.X:F2}, Y={_joystickVector.Y:F2}"
            );
        }
    }

    private void ActualizarBotones()
    {
        if (_btnX == null ||
            _btnY == null ||
            _btnB == null ||
            _btnA == null)
            return;

        if (!_modL1Activo &&
            !_modR1Activo)
        {
            _btnX.Text = "Habilidad 1";
            _btnY.Text = "Habilidad 2";
            _btnB.Text = "Habilidad 3";
            _btnA.Text = "Saltar";

            if (_labelStatus != null)
                _labelStatus.Text =
                    "Set Normal Activo";
        }
        else if (
            _modL1Activo &&
            !_modR1Activo)
        {
            _btnX.Text = "Spell L5";
            _btnY.Text = "Spell L6";
            _btnB.Text = "Spell L7";
            _btnA.Text = "Montura";

            if (_labelStatus != null)
                _labelStatus.Text =
                    "Set L1 Activo";
        }
        else if (
            !_modL1Activo &&
            _modR1Activo)
        {
            _btnX.Text = "Spell R9";
            _btnY.Text = "Spell R10";
            _btnB.Text = "Spell R11";
            _btnA.Text = "Poción";

            if (_labelStatus != null)
                _labelStatus.Text =
                    "Set R1 Activo";
        }
    }

    private void EjecutarJugabilidad(
        int botonId)
    {
        if (
            botonId == 4 &&
            !_modL1Activo &&
            !_modR1Activo
        )
        {
            _network?.EnviarComandoMovimiento(
                "JUMP"
            );

            return;
        }

        int spell = 0;

        if (!_modL1Activo &&
            !_modR1Activo)
        {
            spell = botonId switch
            {
                1 => 47450,
                2 => 47471,
                3 => 47465,
                _ => 0
            };
        }
        else if (
            _modL1Activo &&
            !_modR1Activo)
        {
            spell = botonId switch
            {
                1 => 11578,
                2 => 20252,
                3 => 48068,
                4 => 54729,
                _ => 0
            };
        }
        else if (
            !_modL1Activo &&
            _modR1Activo)
        {
            spell = botonId switch
            {
                1 => 48156,
                2 => 48123,
                3 => 48160,
                4 => 33010,
                _ => 0
            };
        }

        if (spell != 0)
        {
            _network?.EnviarCastSpell(
                spell,
                123456789UL
            );
        }
    }
}
