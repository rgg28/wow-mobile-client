using Godot;

public partial class MobileUI : CanvasLayer
{
    private bool _modificadorActivo = false;
    private NetworkClient _network;

    public override void _Ready()
    {
        _network = GetNode<NetworkClient>("/root/NetworkClient");
    }

    public void _on_modificador_pressed() => _modificadorActivo = true;
    public void _on_modificador_released() => _modificadorActivo = false;

    public void OnBotonHabilidadPressed(int botonId)
    {
        int spellIdALanzar = 0;

        if (!_modificadorActivo)
        {
            // Habilidades normales (ej: Golpe Heroico)
            spellIdALanzar = botonId == 1 ? 47450 : 47471;
        }
        else
        {
            // Habilidades con MOD activo (ej: Cargar)
            spellIdALanzar = botonId == 1 ? 11578 : 20252;
        }

        ulong targetDummyGuid = 123456789;
        _network.EnviarCastSpell(spellIdALanzar, targetDummyGuid);
    }
}
