using Godot;
using System.IO;

public partial class WorldManager : Node3D
{
    public override void _Ready()
    {
        GD.Print("Fase 2: Inicializando entorno 3D de Azeroth...");

        // Evita el bloqueo de Android/data buscando en la carpeta pública Documents
        string rutaAssets = OS.GetName() == "Android" 
            ? "/storage/emulated/0/Documents/WoW335Android/" 
            : "./WoWAssets/";

        if (Directory.Exists(rutaAssets))
        {
            GD.Print($"Assets encontrados en: {rutaAssets}. Cargando mapas 3.3.5a...");
        }
        else
        {
            GD.PrintErr("No se encontraron assets locales. Creando terreno base de prueba.");
            var plane = new MeshInstance3D();
            plane.Mesh = new PlaneMesh { Size = new Vector2(500, 500) };
            AddChild(plane);
        }
    }
}


