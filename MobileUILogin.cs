using Godot;
using System;

public partial class MobileUILogin : Panel
{
    private LineEdit _inputUsuario;
    private LineEdit _inputContrasena;
    private Action<string, string> _onConectarCallback;

    public void Inicializar(Action<string, string> onConectar)
    {
        _onConectarCallback = onConectar;
        AnchorsPreset = 8; // Centrado total
        OffsetLeft = -200; OffsetTop = -150;
        OffsetRight = 200; OffsetBottom = 150;

        var lblTitulo = new Label { 
            Text = "WORLD OF WARCRAFT 3.3.5a", 
            Position = new Vector2(20, 20), 
            HorizontalAlignment = HorizontalAlignment.Center, 
            CustomMinimumSize = new Vector2(360, 30) 
        };
        AddChild(lblTitulo);

        _inputUsuario = new LineEdit { PlaceholderText = "Nombre de Cuenta...", Position = new Vector2(50, 70), CustomMinimumSize = new Vector2(300, 40) };
        AddChild(_inputUsuario);

        _inputContrasena = new LineEdit { PlaceholderText = "Contraseña...", Secret = true, Position = new Vector2(50, 130), CustomMinimumSize = new Vector2(300, 40) };
        AddChild(_inputContrasena);

        var btnConectar = new Button { Text = "CONECTAR", Position = new Vector2(100, 200), CustomMinimumSize = new Vector2(200, 50) };
        btnConectar.Pressed += AlPresionarConectar;
        AddChild(btnConectar);
    }

    private void AlPresionarConectar()
    {
        string user = _inputUsuario.Text.Trim();
        string pass = _inputContrasena.Text;
        if (!string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(pass))
        {
            Visible = false;
            _onConectarCallback?.Invoke(user, pass);
        }
    }
}
