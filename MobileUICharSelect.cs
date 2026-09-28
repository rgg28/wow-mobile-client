using Godot;
using System;
using System.Collections.Generic;

public partial class MobileUICharSelect : Panel
{
    private ItemList _listaPersonajes = null!;
    private Button _btnEntrarMundo = null!;
    private Action<string>? _onPersonajeElegidoCallback;
    private List<string> _personajesCargados = new List<string>();

    public void Inicializar(Action<string> onPersonajeElegido)
    {
        _onPersonajeElegidoCallback = onPersonajeElegido;
        Visible = false;

        // Formato correcto en Godot 4 para anclar a la Derecha-Centro de manera responsiva
        SetAnchorsAndOffsetsPreset(LayoutPreset.CenterRight, LayoutPresetMode.Minsize, margin: 50);
        CustomMinimumSize = new Vector2(300, 500);

        // Usamos un VBoxContainer para alinear los elementos de forma automática y limpia en Android
        var contenedorVertical = new VBoxContainer
        {
            CustomMinimumSize = new Vector2(260, 460),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        // Añadimos un pequeño margen interno (Padding)
        contenedorVertical.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect, LayoutPresetMode.Minsize, margin: 20);
        AddChild(contenedorVertical);

        var lblTitulo = new Label {
            Text = "SELECCIONA UN PERSONAJE",
            HorizontalAlignment = HorizontalAlignment.Center,
            CustomMinimumSize = new Vector2(0, 30)
        };
        contenedorVertical.AddChild(lblTitulo);

        // Lista vertical responsiva para mostrar los personajes
        _listaPersonajes = new ItemList {
            SizeFlagsVertical = SizeFlags.ExpandFill, // Hace que ocupe todo el espacio disponible
            FocusMode = FocusModeEnum.None
        };
        _listaPersonajes.ItemSelected += OnPersonajeSeleccionado;
        contenedorVertical.AddChild(_listaPersonajes);

        // Espaciador para separar la lista del botón inferior
        var separador = new Control { CustomMinimumSize = new Vector2(0, 15) };
        contenedorVertical.AddChild(separador);

        // Botón para confirmar e iniciar el ingreso (Fase 2 - CMSG_PLAYER_LOGIN)
        _btnEntrarMundo = new Button {
            Text = "ENTRAR AL MUNDO",
            CustomMinimumSize = new Vector2(0, 55),
            Disabled = true,
            FocusMode = FocusModeEnum.None
        };
        _btnEntrarMundo.Pressed += OnBtnEntrarMundoPressed;
        contenedorVertical.AddChild(_btnEntrarMundo);
    }

    public void MostrarPersonajes(List<string> listaNombres)
    {
        _personajesCargados = listaNombres;
        _listaPersonajes.Clear();

        foreach (var personaje in _personajesCargados)
        {
            _listaPersonajes.AddItem(personaje);
        }

        Visible = true;
        _btnEntrarMundo.Disabled = true;
    }

    private void OnPersonajeSeleccionado(long index)
    {
        _btnEntrarMundo.Disabled = false;
    }

    private void OnBtnEntrarMundoPressed()
    {
        var seleccionados = _listaPersonajes.GetSelectedItems();
        if (seleccionados.Length > 0)
        {
            int index = seleccionados[0];
            string nombrePersonaje = _personajesCargados[index];
            
            Visible = false; // Ocultar esta pantalla
            GD.Print($"[UI] Personaje elegido: {nombrePersonaje}. Enviando petición de login al mundo...");
            _onPersonajeElegidoCallback?.Invoke(nombrePersonaje);
        }
    }
}
