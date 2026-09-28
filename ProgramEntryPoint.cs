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
        // OS.GetSystemDir obtiene la ruta absoluta hacia 'Documents' de forma nativa en cada celular
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
            // Aquí irá la lógica de inicialización real de tu juego cuando todo esté en orden
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
        AddChild(canvas);
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
        AddChild(canvas);
    }
}
