using Godot;

public partial class MobileUI : CanvasLayer
{
    private bool _modificadorActivo = false;
    private NetworkClient _network;

    public override void _Ready()
    {
        _network = GetNode<NetworkClient>("/root/NetworkClient");
    }

    // El usuario mantiene presionado el botón modificador L1 virtual en pantalla
    public void _on_modificador_pressed() => _modificadorActivo = true;
    public void _on_modificador_released() => _modificadorActivo = false;

    // Se dispara al tocar uno de los 4 botones principales (A, B, X, Y)
    public void OnBotonHabilidadPressed(int botonId)
    {
        int spellIdALanzar = 0;

        if (!_modificadorActivo)
        {
            // Set de habilidades normales (Botones 1 al 4)
            spellIdALanzar = botonId switch { 1 => 47540, 2 => 47458, _ => 0 }; // Ej: Hechizos de Sacerdote/Guerrero
        }
        else
        {
            // Set de habilidades secundarias con el modificador activo (Botones 5 al 8)
            spellIdALanzar = botonId switch { 1 => 48068, 2 => 48156, _ => 0 };
        }

        // Ejecuta la jugabilidad enviando el paquete al servidor a través de la Fase 4
        ulong targetDummyGuid = 123456789;
        _network.EnviarCastSpell(spellIdALanzar, targetDummyGuid);
    }
}
