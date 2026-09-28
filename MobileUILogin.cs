using Godot;
using System;

public partial class MobileUILogin : Panel
{
    private LineEdit _inputUsuario = null!;
    private LineEdit _inputContrasena = null!;
    private Action<string, string>? _onConectarCallback;

    // Método simple para inyectar el callback desde el ProgramEntryPoint antes de añadirlo a la pantalla
    public void ConfigurarCallback(Action<string, string> onConectar)
    {
        _onConectarCallback = onConectar;
    }

    // El motor llama automáticamente a _Ready cuando el nodo se monta de forma real en la pantalla
    public override void _Ready()
    {
        Visible = true;

        // Centrado responsivo oficial de Godot 4 para evitar deformaciones en Android
        SetAnchorsAndOffsetsPreset(LayoutPreset.Center, LayoutPresetMode.Minsize);
        CustomMinimumSize = new Vector2(400, 320);

        // Usamos un VBoxContainer para alinear verticalmente y permitir escalado si el teclado empuja la UI
        var contenedorVertical = new VBoxContainer
        {
            CustomMinimumSize = new Vector2(360, 280),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        contenedorVertical.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect, LayoutPresetMode.Minsize, margin: 20);
        AddChild(contenedorVertical);

        // 1. Título del juego
        var lblTitulo = new Label { 
            Text = "WORLD OF WARCRAFT 3.3.5a", 
            HorizontalAlignment = HorizontalAlignment.Center,
            CustomMinimumSize = new Vector2(0, 40),
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        contenedorVertical.AddChild(lblTitulo);

        // 2. Campo de Usuario
        _inputUsuario = new LineEdit { 
            PlaceholderText = "Nombre de Cuenta...", 
            CustomMinimumSize = new Vector2(0, 45),
            FocusMode = FocusModeEnum.Click,
            VirtualKeyboardType = LineEdit.VirtualKeyboardTypeEnum.Default 
        };
        contenedorVertical.AddChild(_inputUsuario);

        // Pequeño espacio intermedio
        contenedorVertical.AddChild(new Control { CustomMinimumSize = new Vector2(0, 10) });

        // 3. Campo de Contraseña
        _inputContrasena = new LineEdit { 
            PlaceholderText = "Contraseña...", 
            Secret = true, 
            CustomMinimumSize = new Vector2(0, 45),
            FocusMode = FocusModeEnum.Click,
            VirtualKeyboardType = LineEdit.VirtualKeyboardTypeEnum.Password
        };
        contenedorVertical.AddChild(_inputContrasena);

        // Espaciador flexible para empujar el botón hacia abajo de forma limpia
        var espaciador = new Control { SizeFlagsVertical = SizeFlags.ExpandFill };
        contenedorVertical.AddChild(espaciador);

        // 4. Botón de Conectar
        var btnConectar = new Button { 
            Text = "CONECTAR", 
            CustomMinimumSize = new Vector2(0, 55),
            FocusMode = FocusModeEnum.None 
        };
        btnConectar.Pressed += AlPresionarConectar;
        contenedorVertical.AddChild(btnConectar);
    }

    private void AlPresionarConectar()
    {
        string user = _inputUsuario.Text.Trim();
        string pass = _inputContrasena.Text;

        if (!string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(pass))
        {
            Visible = false;
            GD.Print($"[UI] Procesando login para {user.ToUpper()}... Pasando datos al cliente de red.");
            _onConectarCallback?.Invoke(user, pass);
        }
    }
}
