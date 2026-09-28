using Godot;

public partial class ProgramEntryPoint : Node
{
    public override void _Ready()
    {
        // Instancia dinámicamente tus scripts en la memoria del celular
        var worldManager = new WorldManager();
        AddChild(worldManager);

        var mobileUI = new MobileUI();
        AddChild(mobileUI);
        
        GD.Print("[WoW] Cliente iniciado correctamente desde código puro.");
    }
}
