using Godot;
using System;
using System.IO;

public partial class ProgramEntryPoint : Node
{
    private string _rutaDataWoW = "";

    public override void _Ready()
    {
        GD.Print("[WoW] Inicializando cliente móvil 3.3.5a...");

        // 1. Gestionar permisos nativos de almacenamiento en Android
        if (OS.GetName() == "Android")
        {
            // Solicita al usuario los permisos de lectura/escritura mediante el cuadro de diálogo oficial
            OS.RequestPermissions();
        }

        // 2. Establecer la ruta en el almacenamiento interno compartido del dispositivo
        // 'user://' apunta de forma segura a: /Android/data/com.wowandroid.client335/files/
        string rutaBaseApp = ProjectSettings.GlobalizePath("user://");
        _rutaDataWoW = Path.Combine(rutaBaseApp, "Data");

        GD.Print($"[WoW] Buscando directorio de datos del juego en: {_rutaDataWoW}");

        // 3. Verificar si el usuario ya movió sus carpetas de WoW
        if (Directory.Exists(_rutaDataWoW))
        {
            GD.Print("[WoW] ¡Directorio Data detectado! Cargando archivos de Blizzard...");
            InicializarComponentesDelJuego();
        }
        else
        {
            GD.PrintErr("[WoW] [ERROR] Directorio Data ausente.");
            
            // Creamos automáticamente las carpetas para facilitarle la vida al usuario
            try
            {
                Directory.CreateDirectory(_rutaDataWoW);
                GD.Print($"[WoW] Se ha creado la estructura vacía en: {rutaBaseApp}");
            }
            catch (Exception e) 
            { 
                GD.PrintErr($"[WoW] No se pudo crear la estructura de carpetas: {e.Message}"); 
            }

            MostrarMensajeDeFaltaDeArchivos();
        }
    }

    private void InicializarComponentesDelJuego()
    {
        // Aquí conectas tu parser de MPQ y generas tus interfaces
        GD.Print("[WoW] Escaneando e indexando archivos .MPQ (common.mpq, expansion.mpq, patch.mpq)...");
        
        // EJEMPLO de comprobación de archivo crítico antes de procesar:
        string commonMpqPath = Path.Combine(_rutaDataWoW, "common.mpq");
        if (File.Exists(commonMpqPath))
        {
            GD.Print("[WoW] common.mpq encontrado correctamente.");
            // Tu código de inicio o instanciación de UI dinámica de C# aquí
        }
    }

    private void MostrarMensajeDeFaltaDeArchivos()
    {
        // Evita el crash dibujando un aviso en pantalla si el usuario olvidó pasar los datos
        CanvasLayer canvas = new CanvasLayer();
        Label label = new Label();
        
        label.Text = "FALTAN LOS ASSETS DEL JUEGO\n\n" +
                     "Por favor, conecta tu celular a la PC y copia el contenido\n" +
                     "de tu carpeta 'Data' de WoW 3.3.5a dentro de la ruta:\n\n" +
                     "Almacenamiento Interno -> Android -> data -> com.wowandroid.client335 -> files -> Data\n\n" +
                     "Luego, reinicia la aplicación.";
                     
        label.HorizontalAlignment = HorizontalAlignment.Center;
        label.VerticalAlignment = VerticalAlignment.Center;
        label.SetAnchorsPreset(Control.LayoutPreset.FullRect);
        
        // Ajuste básico de tamaño de fuente por código para que sea legible en móviles
        label.AddThemeFontSizeOverride("font_size", 24);
        
        canvas.AddChild(label);
        AddChild(canvas);
    }
}
