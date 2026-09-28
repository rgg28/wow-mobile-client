using Godot;
using System.Collections.Generic;

public partial class MobileUIRadialMenu : Control
{
    private List<string> _opciones = new List<string> { "Personaje", "Inventario", "Hechizos", "Mazmorras", "BGs" };
    private int _seleccionadoIndex = -1;
    private float _radio = 110f;

    public override void _Ready()
    {
        Visible = false;
        float anguloPaso = Mathf.Tau / _opciones.Count;

        for (int i = 0; i < _opciones.Count; i++)
        {
            float anguloActual = i * anguloPaso;
            Vector2 posBoton = new Vector2(Mathf.Cos(anguloActual), Mathf.Sin(anguloActual)) * _radio;

            var btn = new Button {
                Text = _opciones[i],
                CustomMinimumSize = new Vector2(100, 45),
                Position = posBoton - new Vector2(50, 22)
            };
            AddChild(btn);
        }
    }

    public void Mostrar(Vector2 posicionClick)
    {
        Position = posicionClick;
        Visible = true;
        _seleccionadoIndex = -1;
    }

    public int ActualizarArrastre(Vector2 posicionActual, Label labelStatus)
    {
        Vector2 vector = posicionActual - Position;
        if (vector.Length() > 30f)
        {
            float angulo = Mathf.RadToDeg(vector.Angle());
            if (angulo < 0) angulo += 360f;

            float tamañoSector = 360f / _opciones.Count;
            _seleccionadoIndex = Mathf.Clamp((int)(angulo / tamañoSector), 0, _opciones.Count - 1);
            
            labelStatus.Text = $"Selección: {_opciones[_seleccionadoIndex]}";
            return _seleccionadoIndex;
        }
        return -1;
    }

    public int OcultarYSoltar()
    {
        Visible = false;
        int resultado = _seleccionadoIndex;
        _seleccionadoIndex = -1;
        return resultado;
    }
}
