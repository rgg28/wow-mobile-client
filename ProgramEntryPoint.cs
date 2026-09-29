using Godot;

public partial class ProgramEntryPoint : Node
{
    private MobileUI? _mobileUI;

    public override void _Ready()
    {
        GD.Print("=== WoW Android Client ===");
        GD.Print("ProgramEntryPoint iniciado.");

        // MobileUI es el único sistema responsable de la interfaz.
        _mobileUI = new MobileUI();
        AddChild(_mobileUI);

        GD.Print("MobileUI creado correctamente.");
    }
}
