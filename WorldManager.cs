using Godot;
using System;
using System.IO;

public partial class WorldManager : Node3D
{
    // Ruta en Android donde debes copiar tus assets extraídos del cliente 3.3.5a
    // Ej: /Android/data/com.wowandroid.client335/files/WoWAssets/
    private string _rutaAssetsAndroid = "user://WoWAssets/";

    public override void _Ready()
    {
        GD.Print("Fase 2: Inicializando entorno 3D de Azeroth...");
        
        // Verificar si estamos en Android o en PC de desarrollo
        string rutaFinal = OS.GetName() == "Android" ? ProjectSettings.GlobalizePath(_rutaAssetsAndroid) : "./WoWAssets/";

        if (Directory.Exists(rutaFinal))
        {
            GD.Print($"Assets encontrados en: {rutaFinal}. Cargando mapas del cliente...");
            CargarMapasOriginales(rutaFinal);
        }
        else
        {
            GD.PrintErr($"Alerta: No se encontraron assets en {rutaFinal}. Creando entorno de pruebas.");
            GenerarTerrenoBase();
        }
    }

    private void CargarMapasOriginales(string ruta)
    {
        // Aquí tu motor leerá los archivos .obj, .gltf o binarios que extraigas de tus .MPQ
        GD.Print("Cargando geometrías tridimensionales de Azeroth 3.3.5a...");
        
        // Ejemplo de instanciación dinámica de un asset si ya lo tienes convertido:
        // var modeloMesh = GD.Load<PackedScene>("res://WoWAssets/Mundo.tscn").Instantiate();
        // AddChild(modeloMesh);
    }

    private void GenerarTerrenoBase()
    {
        // Terreno de respaldo en caso de que no se encuentren los archivos en el celular
        var plane = new MeshInstance3D();
        plane.Mesh = new PlaneMesh { Size = new Vector2(500, 500) };
        
        // Crear un material simple para el suelo
        var material = new StandardMaterial3D();
        material.AlbedoColor = new Color(0.1f, 0.4f, 0.1f); // Verde texturizado simulado
        plane.MaterialOverride = material;

        AddChild(plane);
        GD.Print("Entorno tridimensional de prueba generado.");
    }
}
