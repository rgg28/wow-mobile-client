using Godot;
using System.Collections.Generic;

public partial class MobileUIRadialMenu : Control
{
    private List<string> _opciones = new List<string> { "Personaje", "Inventario", "Hechizos", "Mazmorras", "BGs" };
    private int _seleccionadoIndex = -1;
    private float _radio = 110f;
    private bool _activo = false;
    private Label? _labelStatus; // Referencia interna opcional

    public override void _Ready()
    {
        Visible = false;
        
        // Buscamos un label de estado en la escena de forma automática si existe
        _labelStatus = GetNodeOrNull<Label>("../LabelStatus");

        float anguloPaso = Mathf.Tau / _opciones.Count;

        for (int i = 0; i < _opciones.Count; i++)
        {
            float anguloActual = i * anguloPaso;
            // Calculamos la posición radial respecto al centro (0,0) del control
            Vector2 posBoton = new Vector2(Mathf.Cos(anguloActual), Mathf.Sin(anguloActual)) * _radio;

            var btn = new Button {
                Text = _opciones[i],
                CustomMinimumSize = new Vector2(100, 45),
                // Centramos el botón en su coordenada radial
                Position = posBoton - new Vector2(50, 22),
                FocusMode = FocusModeEnum.None // Evita que interfiera con el arrastre táctil
            };
            AddChild(btn);
        }
    }

    // Captura los eventos táctiles nativos de Android de forma automática
    public override void _Input(InputEvent @event)
    {
        // 1. Detectar clic largo o pulsación en la pantalla para abrir el menú (Ejemplo con click derecho o toque largo simulado)
        if (@event is InputEventScreenTouch touchEvent)
        {
            if (touchEvent.Pressed)
            {
                // Aquí podrías poner una condición de pulsación larga. Por ahora abre en la posición del toque.
                if (!Visible) 
                {
                    Mostrar(touchEvent.Position);
                }
            }
            else if (!touchEvent.Pressed && _activo)
            {
                // Al levantar el dedo, ocultamos y procesamos la selección
                int seleccionFinal = OcultarYSoltar();
                EjecutarAccionWoW(seleccionFinal);
            }
        }

        // 2. Gestionar el arrastre del dedo por la pantalla
        if (@event is InputEventScreenDrag dragEvent && _activo)
        {
            ActualizarArrastre(dragEvent.Position);
        }
    }

    public void Mostrar(Vector2 posicionClick)
    {
        GlobalPosition = posicionClick; // Usamos GlobalPosition para alinearlo perfectamente con el dedo
        Visible = true;
        _activo = true;
        _seleccionadoIndex = -1;
        if (_labelStatus != null) _labelStatus.Text = "Selección: Ninguna";
    }

    public int ActualizarArrastre(Vector2 posicionActual)
    {
        // Calculamos el vector relativo usando la posición global del menú
        Vector2 vector = posicionActual - GlobalPosition;

        if (vector.Length() > 40f) // Incrementamos a 40f como zona muerta central para evitar selecciones accidentales
        {
            float angulo = Mathf.RadToDeg(vector.Angle());
            if (angulo < 0) angulo += 360f;

            float tamanoSector = 360f / _opciones.Count;
            _seleccionadoIndex = Mathf.Clamp((int)(angulo / tamanoSector), 0, _opciones.Count - 1);
            
            if (_labelStatus != null)
            {
                _labelStatus.Text = $"Selección: {_opciones[_seleccionadoIndex]}";
            }
            return _seleccionadoIndex;
        }
        
        _seleccionadoIndex = -1;
        return -1;
    }

    public int OcultarYSoltar()
    {
        Visible = false;
        _activo = false;
        int resultado = _seleccionadoIndex;
        _seleccionadoIndex = -1;
        return resultado;
    }

    private void EjecutarAccionWoW(int index)
    {
        if (index == -1) return;

        string opcion = _opciones[index];
        GD.Print($"[UI] Ejecutando acción de interfaz de WoW para: {opcion}");

        // Accedemos de forma segura al Autoload de Red que corregimos antes
        var netClient = GetNodeOrNull<NetworkClient>("/root/NetworkClient");
        if (netClient != null)
        {
            netClient.EnviarComandoInterfaz(opcion);
        }
    }
}
