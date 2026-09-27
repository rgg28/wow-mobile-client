using Godot;

public partial class WorldManager : Node3D
{
    public override void _Ready()
    {
        GD.Print("Fase 2: Inicializando entorno 3D de Azeroth...");
        GenerarTerrenoBase();
    }

    private void GenerarTerrenoBase()
    {
        // Reemplazar en desarrollo con cargador de archivos .ADT / .WMO de WoW
        var plane = new MeshInstance3D();
        plane.Mesh = new PlaneMesh { Size = new Vector2(500, 500) };
        AddChild(plane);
        GD.Print("Entorno tridimensional base listo.");
    }
}
