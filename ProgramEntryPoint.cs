using Godot;
using System;

public partial class ProgramEntryPoint : Node
{
    private MobileUI? _mobileUI;
    private CanvasLayer? _diagnosticLayer;

    public override void _Ready()
    {
        GD.Print("========================================");
        GD.Print("[WoW] WoW Mobile Client");
        GD.Print("[WoW] Inicializando Android client...");
        GD.Print("========================================");

        try
        {
            CrearInterfazPrincipal();

            GD.Print("[WoW] Bootstrap completado correctamente.");
        }
        catch (Exception ex)
        {
            GD.PrintErr("[WoW] ERROR DURANTE EL ARRANQUE");
            GD.PrintErr(ex.ToString());

            MostrarErrorFatal(ex);
        }
    }

    private void CrearInterfazPrincipal()
    {
        Node root = GetTree().Root;

        GD.Print("[WoW] Creando MobileUI...");

        _mobileUI = new MobileUI();

        root.AddChild(_mobileUI);

        GD.Print("[WoW] MobileUI agregado correctamente.");

        MostrarEstadoInicial();
    }

    private void MostrarEstadoInicial()
    {
        CanvasLayer canvas = new CanvasLayer();
        canvas.Name = "BootStatus";

        Label label = new Label();

        label.Text =
            "WORLD OF WARCRAFT 3.3.5a\n\n" +
            "WoW Mobile Client\n\n" +
            "Inicialización completada.\n\n" +
            "Selecciona tus datos del cliente\n" +
            "cuando el sistema de assets esté habilitado.";

        label.HorizontalAlignment = HorizontalAlignment.Center;
        label.VerticalAlignment = VerticalAlignment.Center;

        label.SetAnchorsPreset(Control.LayoutPreset.FullRect);

        label.AddThemeFontSizeOverride("font_size", 22);

        canvas.AddChild(label);

        GetTree().Root.AddChild(canvas);

        _diagnosticLayer = canvas;

        /*
         * No eliminamos MobileUI.
         *
         * Este mensaje solamente sirve para comprobar que:
         *
         * Godot -> C# -> ProgramEntryPoint -> MobileUI
         *
         * funciona correctamente en Android.
         *
         * El lector de assets se habilitará posteriormente.
         */
    }

    private void MostrarErrorFatal(Exception ex)
    {
        try
        {
            CanvasLayer canvas = new CanvasLayer();
            canvas.Name = "FatalError";

            ColorRect fondo = new ColorRect();

            fondo.SetAnchorsPreset(Control.LayoutPreset.FullRect);

            Label label = new Label();

            label.Text =
                "ERROR DE INICIALIZACIÓN\n\n" +
                "El cliente no pudo iniciar.\n\n" +
                ex.GetType().Name +
                "\n\n" +
                ex.Message;

            label.HorizontalAlignment = HorizontalAlignment.Center;
            label.VerticalAlignment = VerticalAlignment.Center;

            label.SetAnchorsPreset(Control.LayoutPreset.FullRect);

            label.AddThemeFontSizeOverride("font_size", 20);

            canvas.AddChild(fondo);
            canvas.AddChild(label);

            GetTree().Root.AddChild(canvas);
        }
        catch
        {
            GD.PrintErr("[WoW] No se pudo mostrar el error en pantalla.");
        }
    }
}
