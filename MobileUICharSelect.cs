using Godot;
using System;
using System.Collections.Generic;

public partial class MobileUICharSelect : Panel
{
    private ItemList _listaPersonajes;
    private Button _btnEntrarMundo;
    private Action<string> _onPersonajeElegidoCallback;
    private List<string> _personajesCargados = new List<string>();

    public void Inicializar(Action<string> onPersonajeElegido)
    {
        _onPersonajeElegidoCallback = onPersonajeElegido;

        // Configurar tamaño del panel central de selección (Lado derecho de la pantalla, estilo WoW)
        AnchorsPreset = 6; // Derecha - Centro
        OffsetLeft = -350; OffsetTop = -250;
        OffsetRight = -50; OffsetBottom = 250;
        Visible = false;

        var lblTitulo = new Label {
            Text = "SELECCIONA UN PERSONAJE",
            Position = new Vector2(20, 20),
            CustomMinimumSize = new Vector2(260, 30),
            HorizontalAlignment = HorizontalAlignment.Center
        };
        AddChild(lblTitulo);

        // Lista vertical para mostrar los personajes
        _listaPersonajes = new ItemList {
            Position = new Vector2(20, 60),
            CustomMinimumSize = new Vector2(260, 300)
        };
        _listaPersonajes.ItemSelected += OnPersonajeSeleccionado;
        AddChild(_listaPersonajes);

        // Botón para confirmar e iniciar la Fase 2 (Entrar al mundo)
        _btnEntrarMundo = new Button {
            Text = "ENTRAR AL MUNDO",
            Position = new Vector2(20, 380),
            CustomMinimumSize = new Vector2(260, 50),
            Disabled = true
        };
        _btnEntrarMundo.Pressed += OnBtnEntrarMundoPressed;
        AddChild(_btnEntrarMundo);
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
            _onPersonajeElegidoCallback?.Invoke(nombrePersonaje);
        }
    }
}
