using Godot;

public partial class MobileUI : CanvasLayer
{
    private bool _modificadorActivo = false;
    private NetworkClient _network;
    private Label _labelStatus;
    private Button _btnA;
    private Button _btnB;

    public override void _Ready()
    {
        _network = GetNode<NetworkClient>("/root/NetworkClient");

        // 1. Crear el contenedor táctil que ocupa toda la pantalla del móvil
        var control = new Control { LayoutMode = 1, AnchorsPreset = 15 };
        AddChild(control);

        // 2. Crear el texto de estado superior
        _labelStatus = new Label {
            Text = "Conectando al protocolo 3.3.5a...",
            HorizontalAlignment = HorizontalAlignment.Center,
            AnchorsPreset = 5, // Arriba - Centro
            OffsetTop = 40, OffsetLeft = -200, OffsetRight = 200
        };
        control.AddChild(_labelStatus);

        // 3. Crear el Botón Modificador L1 (Lado Izquierdo)
        var btnMod = new Button {
            Text = "MOD (L1)",
            AnchorsPreset = 4, // Izquierda - Centro
            OffsetLeft = 60, OffsetTop = -45, 
            CustomMinimumSize = new Vector2(150, 90)
        };
        btnMod.ButtonDown += () => { _modificadorActivo = true; ActualizarBotones(); };
        btnMod.ButtonUp += () => { _modificadorActivo = false; ActualizarBotones(); };
        control.AddChild(btnMod);

        // 4. Crear la rejilla para agrupar los botones de ataque (Lado Derecho)
        var grid = new GridContainer {
            Columns = 2,
            AnchorsPreset = 6, // Derecha - Centro
            OffsetLeft = -280, OffsetTop = -45
        };
        control.AddChild(grid);

        // Botón de Habilidad A
        _btnA = new Button { CustomMinimumSize = new Vector2(120, 90) };
        _btnA.Pressed += () => EjecutarJugabilidad(1);
        grid.AddChild(_btnA);

        // Botón de Habilidad B
        _btnB = new Button { CustomMinimumSize = new Vector2(120, 90) };
        _btnB.Pressed += () => EjecutarJugabilidad(2);
        grid.AddChild(_btnB);

        ActualizarBotones();
    }

    private void ActualizarBotones()
    {
        if (!_modificadorActivo)
        {
            _btnA.Text = "Golpe Heroico";
            _btnB.Text = "Ejecutar";
            _labelStatus.Text = "Set de habilidades normales activo";
        }
        else
        {
            _btnA.Text = "Cargar (Mod)";
            _btnB.Text = "Intervenir (Mod)";
            _labelStatus.Text = "Set MODIFICADOR (L1) retenido";
        }
    }

    private void EjecutarJugabilidad(int botonId)
    {
        int spellId = 0;
        if (!_modificadorActivo)
        {
            spellId = botonId == 1 ? 47450 : 47471; // Opcodes 3.3.5a
        }
        else
        {
            spellId = botonId == 1 ? 11578 : 20252; // Opcodes modificados
        }

        ulong targetDummyGuid = 123456789;
        _network.EnviarCastSpell(spellId, targetDummyGuid);
    }
}
