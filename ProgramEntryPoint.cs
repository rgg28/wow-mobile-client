using Godot;

public partial class ProgramEntryPoint : Node
{
    public override void _Ready()
    {
        GD.Print("[WoW] Iniciando componentes gráficos y lógicos...");

        // 1. Cargar e instanciar la escena visual de la interfaz móvil
        var mobileUIScene = GD.Load<PackedScene>("res://MobileUI.tscn");
        var mobileUI = mobileUIScene.Instantiate();
        AddChild(mobileUI);

        // 2. Cargar e instanciar el gestor del mundo
        var worldManagerScene = GD.Load<PackedScene>("res://WorldManager.tscn");
        var worldManager = worldManagerScene.Instantiate();
        AddChild(worldManager);
        
        GD.Print("[WoW] Cliente iniciado correctamente con interfaz gráfica.");
    }
}
