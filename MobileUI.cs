using Godot;

public partial class MobileUI : CanvasLayer
{
    private bool _modL1Activo = false;
    private bool _modR1Activo = false;
    private NetworkClient _network;
    private Label _labelStatus;
    
    private Button _btnA, _btnB, _btnX, _btnY;
    private Control _joystickBase;
    private Button _joystickPivote;
    private bool _joystickTocado = false;
    private Vector2 _joystickVector = Vector2.Zero;
    private float _joystickRadioMax = 50f;

    // Conexión con el nuevo archivo del menú radial
    private MobileUIRadialMenu _menuRadial;
    private bool _menuRadialActivo = false;

    public override void _Ready()
    {
        _network = GetNode<NetworkClient>("/root/NetworkClient");

        var control = new Control { LayoutMode = 1, AnchorsPreset = 15 };
        AddChild(control);

        _labelStatus = new Label {
            Text = "Conectando al protocolo 3.3.5a...",
            HorizontalAlignment = HorizontalAlignment.Center,
            AnchorsPreset = 5, OffsetTop = 40, OffsetLeft = -200, OffsetRight = 200
        };
        control.AddChild(_labelStatus);

        // Modificadores L1 y R1
        var btnL1 = new Button { Text = "MOD (L1)", AnchorsPreset = 4, OffsetLeft = 60, OffsetTop = -180, CustomMinimumSize = new Vector2(140, 70) };
        btnL1.ButtonDown += () => { _modL1Activo = true; ActualizarBotones(); };
        btnL1.ButtonUp += () => { _modL1Activo = false; ActualizarBotones(); };
        control.AddChild(btnL1);

        var btnR1 = new Button { Text = "MOD (R1)", AnchorsPreset = 6, OffsetLeft = -200, OffsetTop = -180, CustomMinimumSize = new Vector2(140, 70) };
        btnR1.ButtonDown += () => { _modR1Activo = true; ActualizarBotones(); };
        btnR1.ButtonUp += () => { _modR1Activo = false; ActualizarBotones(); };
        control.AddChild(btnR1);

        // Joystick Virtual
        _joystickBase = new Control { AnchorsPreset = 2, OffsetLeft = 100, OffsetTop = -200, CustomMinimumSize = new Vector2(120, 120) };
        control.AddChild(_joystickBase);
        _joystickBase.AddChild(new Panel { CustomMinimumSize = new Vector2(120, 120) });

        _joystickPivote = new Button { CustomMinimumSize = new Vector2(50, 50), Position = new Vector2(35, 35) };
        _joystickPivote.GuiInput += OnJoystickGuiInput;
        _joystickBase.AddChild(_joystickPivote);

        // Panel de Ataque Rombo
        var panelBotones = new Control { AnchorsPreset = 3, OffsetLeft = -300, OffsetTop = -220 };
        control.AddChild(panelBotones);

        _btnX = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(0, 70) }; _btnX.Pressed += () => EjecutarJugabilidad(1); panelBotones.AddChild(_btnX);
        _btnY = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(80, 0) }; _btnY.Pressed += () => EjecutarJugabilidad(2); panelBotones.AddChild(_btnY);
        _btnB = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(160, 70) }; _btnB.Pressed += () => EjecutarJugabilidad(3); panelBotones.AddChild(_btnB);
        _btnA = new Button { CustomMinimumSize = new Vector2(80, 80), Position = new Vector2(80, 140) }; _btnA.Pressed += () => EjecutarJugabilidad(4); panelBotones.AddChild(_btnA);

        // Instanciar el menú radial desde el segundo archivo
        _menuRadial = new MobileUIRadialMenu();
        control.AddChild(_menuRadial);

        ActualizarBotones();
    }

    public override void _Input(InputEvent @event)
    {
        if (@event is InputEventScreenTouch touchEvent)
        {
            float tercioAncho = GetViewport().GetVisibleRect().Size.X / 3;
            bool enElCentro = touchEvent.Position.X > tercioAncho && touchEvent.Position.X < (tercioAncho * 2);

            if (touchEvent.Pressed && enElCentro && !_joystickTocado)
            {
                _menuRadialActivo = true;
                _menuRadial.Mostrar(touchEvent.Position);
                _labelStatus.Text = "Desliza para elegir un menú...";
            }
            else if (!touchEvent.Pressed && _menuRadialActivo)
            {
                _menuRadialActivo = false;
                int seleccion = _menuRadial.OcultarYSoltar();
                ProcesarSeleccionRadial(seleccion);
                ActualizarBotones();
            }
        }
        else if (@event is InputEventScreenDrag dragEvent && _menuRadialActivo)
        {
            _menuRadial.ActualizarArrastre(dragEvent.Position, _labelStatus);
        }
    }

    private void ProcesarSeleccionRadial(int index)
    {
        if (index == -1) return;
        string token = index switch {
            0 => "TOGGLE_CHARACTER_SHEET",
            1 => "TOGGLE_BAGS",
            2 => "TOGGLE_SPELLBOOK",
            3 => "TOGGLE_LFG_PARENT",
            4 => "TOGGLE_BATTLEGROUND",
            _ => ""
        };
        if (token != "") _network.EnviarComandoInterfaz(token);
    }

    private void OnJoystickGuiInput(InputEvent @event)
    {
        if (@event is InputEventScreenTouch touchEvent)
        {
            _joystickTocado = touchEvent.Pressed;
            if (!_joystickTocado) { _joystickVector = Vector2.Zero; _joystickPivote.Position = new Vector2(35, 35); }
        }
        else if (@event is InputEventScreenDrag dragEvent && _joystickTocado)
        {
            Vector2 centroBase = new Vector2(60, 60);
            Vector2 vector = _joystickBase.GetLocalMousePosition() - centroBase;
            if (vector.Length() > _joystickRadioMax) vector = vector.Normalized() * _joystickRadioMax;
            _joystickVector = vector / _joystickRadioMax;
            _joystickPivote.Position = (centroBase + vector) - new Vector2(25, 25);
        }
    }

    public override void _PhysicsProcess(double delta)
    {
        if (_joystickVector != Vector2.Zero && !_menuRadialActivo)
        {
            GD.Print($"[Movimiento] X={_joystickVector.X:F2}, Y={_joystickVector.Y:F2}");
        }
    }

    private void ActualizarBotones()
    {
        if (!_modL1Activo && !_modR1Activo)
        {
            _btnX.Text = "Habilidad 1"; _btnY.Text = "Habilidad 2"; _btnB.Text = "Habilidad 3"; _btnA.Text = "Saltar";
            _labelStatus.Text = "Set Normal Activo";
        }
        else if (_modL1Activo && !_modR1Activo)
        {
            _btnX.Text = "Spell L5"; _btnY.Text = "Spell L6"; _btnB.Text = "Spell L7"; _btnA.Text = "Montura";
            _labelStatus.Text = "Set Modificador L1 Activo";
        }
        else if (!_modL1Activo && _modR1Activo)
        {
            _btnX.Text = "Spell R9"; _btnY.Text = "Spell R10"; _btnB.Text = "Spell R11"; _btnA.Text = "Poción";
            _labelStatus.Text = "Set Modificador R1 Activo";
        }
    }

    private void EjecutarJugabilidad(int botonId)
    {
        if (botonId == 4 && !_modL1Activo && !_modR1Activo) { _network.EnviarComandoMovimiento("JUMP"); return; }
        
        int spellId = 0;
        if (!_modL1Activo && !_modR1Activo) spellId = botonId switch { 1 => 47450, 2 => 47471, 3 => 47465, _ => 0 };
        else if (_modL1Activo && !_modR1Activo) spellId = botonId switch { 1 => 11578, 2 => 20252, 3 => 48068, 4 => 54729, _ => 0 };
        else if (!_modL1Activo && _modR1Activo) spellId = botonId switch { 1 => 48156, 2 => 48123, 3 => 48160, 4 => 33010, _ => 0 };

        if (spellId != 0) _network.EnviarCastSpell(spellId, 123456789L);
    }
}
