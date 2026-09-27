using Godot;

public partial class ProgramEntryPoint : Node
{
    public override void _Ready()
    {
        GD.Print("Iniciando WoW Android Client puramente desde código C#...");

        // 1. Instanciar el manejador del mundo 3D
        var worldManager = new WorldManager();
        worldManager.Name = "WorldManager";
        AddChild(worldManager);

        // 2. Instanciar la interfaz de usuario móvil nativa de Godot
        var mobileUI = new MobileUI();
        mobileUI.Name = "MobileUI";
        AddChild(mobileUI);
        
        GD.Print("Estructura de juego inyectada con éxito.");
    }
}
