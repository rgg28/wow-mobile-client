using Godot;

public partial class MobileUI : CanvasLayer
{
    private bool _modL1Activo = false;
    private bool _modR1Activo = false;
    private NetworkClient _network;
    private Label _labelStatus;
    
    // Los 4 botones de habilidades principales
    private Button _btnA, _btnB, _btnX, _btnY;

    // Variables para el Joystick Virtual
    private Control _joystickBase;
    private Button _joystickPivote;
    private bool _joystickTocado = false;
    private Vector2 _joystickVector = Vector2.Zero;
    private float _joystickRadioMax = 50f;

    public override void _Ready()
    {
        _network = GetNode<NetworkClient>("/root/NetworkClient");

        // 1. Contenedor principal de la pantalla
        var control = new Control { LayoutMode = 1, AnchorsPreset = 15 };
        AddChild(control);

        // 2. Texto de estado superior
        _labelStatus = new Label {
            Text = "Conectando al protocolo 3.3.5a...",
            HorizontalAlignment = HorizontalAlignment.Center,
            AnchorsPreset = 5,
            OffsetTop = 40, OffsetLeft = -200, OffsetRight = 200
        };
        control.AddChild(_labelStatus);

        // 3. Botón Modificador L1 (Arriba a la izquierda)
        var btnL1 = new Button {
            Text = "MOD (L1)",
            AnchorsPreset = 4, // Izquierda - Centro
            OffsetLeft = 60, OffsetTop = -180, 
            CustomMinimumSize = new Vector2(140, 70)
        };
        btnL1.ButtonDown += () => { _modL1Activo = true; ActualizarBotones(); };
        btnL1.ButtonUp += () => { _modL1Activo = false; ActualizarBotones(); };
        control.AddChild(btnL1);

        // 4. Botón Modificador R1 (Arriba a la derecha, sobre los botones de ataque)
        var btnR1 = new Button {
            Text = "MOD (R1)",
            AnchorsPreset = 6, // Derecha - Centro
            OffsetLeft = -200, OffsetTop = -180, 
            CustomMinimumSize = new Vector2(140, 70)
        };
        btnR1.ButtonDown += () => { _modR1Activo = true; ActualizarBotones(); };
        btnR1.ButtonUp += () => { _modR1Activo = false; ActualizarBotones(); };
        control.AddChild(btnR1);

        // 5. CREACIÓN DEL JOYSTICK VIRTUAL (Abajo a la izquierda)
        _joystickBase = new Control {
            AnchorsPreset = 2, // Abajo - Izquierda
            OffsetLeft = 100, OffsetTop = -200,
            CustomMinimumSize = new Vector2(120, 120)
        };
        control.AddChild(_joystickBase);

        var panelFondoJoy = new Panel {
            CustomMinimumSize = new Vector2(120, 120)
        };
        _joystickBase.AddChild(panelFondoJoy);

        _joystickPivote = new Button {
            CustomMinimumSize = new Vector2(50, 50),
            Position = new Vector2(35, 35)
        };
        _joystickPivote.GuiInput += OnJoystickGuiInput;
        _joystickBase.AddChild(_joystickPivote);

        // 6. LOS 4 BOTONES EN FORMA DE ROMBO (Abajo a la derecha)
        var panelBotones = new Control {
            AnchorsPreset = 3, // Abajo - Derecha
            OffsetLeft = -300, OffsetTop = -220
        };
        control.AddChild(panelBotones);

        _btnX = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(0, 70) };
        _btnX.Pressed += () => EjecutarJugabilidad(1);
        panelBotones.AddChild(_btnX);

        _btnY = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(80, 0) };
        _btnY.Pressed += () => EjecutarJugabilidad(2);
        panelBotones.AddChild(_btnY);

        _btnB = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(160, 70) };
        _btnB.Pressed += () => EjecutarJugabilidad(3);
        panelBotones.AddChild(_btnB);

        _btnA = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(80, 140) };
        _btnA.Pressed += () => EjecutarJugabilidad(4);
        panelBotones.AddChild(_btnA);

        ActualizarBotones();
    }

    private void OnJoystickGuiInput(InputEvent @event)
    {
        if (@event is InputEventScreenTouch touchEvent)
        {
            _joystickTocado = touchEvent.Pressed;
            if (!_joystickTocado)
            {
                _joystickVector = Vector2.Zero;
                _joystickPivote.Position = new Vector2(35, 35);
            }
        }
        else if (@event is InputEventScreenDrag dragEvent && _joystickTocado)
        {
            Vector2 centroBase = new Vector2(60, 60);
            Vector2 posicionToqueLocal = _joystickBase.GetLocalMousePosition();
            Vector2 vectorDesdeCentro = posicionToqueLocal - centroBase;

            if (vectorDesdeCentro.Length() > _joystickRadioMax)
            {
                vectorDesdeCentro = vectorDesdeCentro.Normalized() * _joystickRadioMax;
            }

            _joystickVector = vectorDesdeCentro / _joystickRadioMax;
            _joystickPivote.Position = (centroBase + vectorDesdeCentro) - new Vector2(25, 25);
        }
    }

    public override void _PhysicsProcess(double delta)
    {
        if (_joystickVector != Vector2.Zero)
        {
            GD.Print($"[Movimiento] X={_joystickVector.X:F2}, Y={_joystickVector.Y:F2}");
        }
    }

    private void ActualizarBotones()
    {
        // Estado 1: Ningún modificador presionado
        if (!_modL1Activo && !_modR1Activo)
        {
            _btnX.Text = "Ataque 1";
            _btnY.Text = "Ataque 2";
            _btnB.Text = "Ataque 3";
            _btnA.Text = "Saltar";
            _labelStatus.Text = "Set Normal Activo";
        }
        // Estado 2: Manteniendo presionado L1
        else if (_modL1Activo && !_modR1Activo)
        {
            _btnX.Text = "Spell L5";
            _btnY.Text = "Spell L6";
            _btnB.Text = "Spell L7";
            _btnA.Text = "Montura";
            _labelStatus.Text = "Set MODIFICADOR L1 Activo";
        }
        // Estado 3: Manteniendo presionado R1
        else if (!_modL1Activo && _modR1Activo)
        {
            _btnX.Text = "Spell R9";
            _btnY.Text = "Spell R10";
            _btnB.Text = "Spell R11";
            _btnA.Text = "Poción";
            _labelStatus.Text = "Set MODIFICADOR R1 Activo";
        }
    }

    private void EjecutarJugabilidad(int botonId)
    {
        int spellId = 0;
        
        // Mapeo triple de habilidades (Total: 12 acciones posibles)
        if (!_modL1Activo && !_modR1Activo)
        {
            spellId = botonId switch { 1 => 47450, 2 => 47471, 3 => 47465, 4 => 0, _ => 0 }; // Normales
        }
        else if (_modL1Activo && !_modR1Activo)
        {
            spellId = botonId switch { 1 => 11578, 2 => 20252, 3 => 48068, 4 => 54729, _ => 0 }; // Con L1
        }
        else if (!_modL1Activo && _modR1Activo)
        {
            spellId = botonId switch { 1 => 48156, 2 => 48123, 3 => 48160, 4 => 33010, _ => 0 }; // Con R1
        }

        if (spellId != 0)
        {
            ulong targetDummyGuid = 123456789;
            _network.EnviarCastSpell(spellId, targetDummyGuid);
        }
    }
}
