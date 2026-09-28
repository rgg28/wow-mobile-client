using Godot;
using System;
using System.IO;

public partial class ProgramEntryPoint : Node
{
    private string _rutaAssets = "";

    public override void _Ready()
    {
        GD.Print("[WoW] Inicializando cliente móvil 3.3.5a...");

        // 1. Solicitar permisos de almacenamiento si estamos en Android
        if (OS.GetName() == "Android")
        {
            OS.RequestPermissions();
        }

        // 2. Construir la ruta exacta definida: Almacenamiento interno/Documents/WoW335Android/
        string rutaDocuments = OS.GetSystemDir(OS.SystemDir.Documents);
        _rutaAssets = Path.Combine(rutaDocuments, "WoW335Android");

        GD.Print($"[WoW] Buscando los datos del cliente en: {_rutaAssets}");

        // 3. Validar si la carpeta existe
        if (Directory.Exists(_rutaAssets))
        {
            GD.Print("[WoW] ¡Directorio WoW335Android detectado con éxito!");
            VerificarCarpetasInternas();
        }
        else
        {
            GD.PrintErr($"[WoW] [ERROR] No se encontró la carpeta en: {_rutaAssets}");
            
            // Intentamos crear la carpeta automáticamente para facilitarle el trabajo al usuario
            try
            {
                Directory.CreateDirectory(_rutaAssets);
                GD.Print($"[WoW] Se ha creado la carpeta vacía en: {_rutaAssets}");
            }
            catch (Exception e) 
            { 
                GD.PrintErr($"[WoW] No se pudo crear el directorio: {e.Message}"); 
            }

            MostrarMensajeErrorRuta();
        }
    }

    private void VerificarCarpetasInternas()
    {
        // Validamos las carpetas que extrajiste de los MPQ dentro de tu ruta específica
        string carpetaTexturas = Path.Combine(_rutaAssets, "Textures");
        string carpetaWorld = Path.Combine(_rutaAssets, "World");

        if (Directory.Exists(carpetaTexturas) && Directory.Exists(carpetaWorld))
        {
            GD.Print("[WoW] Carpetas 'Textures' y 'World' verificadas. Iniciando lectura binaria de assets...");
            
            // 🚀 Inicialización Dinámica del Juego sin Escenas:
            // Al compilar de manera pura en C#, instanciamos nuestra interfaz de Login con 'new'
            // y la colgamos directamente de la ventana raíz del motor para que se dibuje.
            var root = GetTree().Root;
            MobileUILogin pantallaLogin = new MobileUILogin();
            root.AddChild(pantallaLogin);
        }
        else
        {
            GD.PrintErr("[WoW] [ERROR] Estructura interna incompleta dentro de WoW335Android.");
            MostrarMensajeErrorEstructura();
        }
    }

    private void MostrarMensajeErrorRuta()
    {
        CanvasLayer canvas = new CanvasLayer();
        Label label = new Label();
        
        label.Text = "FALTAN LOS ARCHIVOS DEL JUEGO\n\n" +
                     "Por favor, copia tus assets extraídos en la memoria de tu móvil:\n\n" +
                     "Almacenamiento interno ➡️ Documents ➡️ WoW335Android ➡️ (Aquí tus carpetas)\n\n" +
                     "Luego, reinicia la aplicación.";
                     
        label.HorizontalAlignment = HorizontalAlignment.Center;
        label.VerticalAlignment = VerticalAlignment.Center;
        label.SetAnchorsPreset(Control.LayoutPreset.FullRect);
        label.AddThemeFontSizeOverride("font_size", 22);
        
        canvas.AddChild(label);
        
        // 🛠️ CORRECCIÓN: Forzamos el renderizado inyectando el Canvas directamente al Root de Godot
        GetTree().Root.AddChild(canvas);
    }

    private void MostrarMensajeErrorEstructura()
    {
        CanvasLayer canvas = new CanvasLayer();
        Label label = new Label();
        
        label.Text = "ESTRUCTURA DE ASSETS INCORRECTA\n\n" +
                     "Asegúrate de que dentro de 'WoW335Android' se encuentren\n" +
                     "las carpetas extraídas directamente de los archivos MPQ\n" +
                     "(por ejemplo, las carpetas 'Textures' y 'World').";
                     
        label.HorizontalAlignment = HorizontalAlignment.Center;
        label.VerticalAlignment = VerticalAlignment.Center;
        label.SetAnchorsPreset(Control.LayoutPreset.FullRect);
        label.AddThemeFontSizeOverride("font_size", 22);
        
        canvas.AddChild(label);
        
        // 🛠️ CORRECCIÓN: Forzamos el renderizado inyectando el Canvas directamente al Root de Godot
        GetTree().Root.AddChild(canvas);
    }
}
