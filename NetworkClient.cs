using Godot;
using System;
using System.Net.Sockets;

public partial class NetworkClient : Node
{
    private TcpClient _socket;
    private NetworkStream _stream;
    private string _authServer = "127.0.0.1"; // Cambiar por tu IP de AzerothCore
    private int _port = 3724;

    public override void _Ready()
    {
        ConectarAlServidor();
    }

    public void ConectarAlServidor()
    {
        try
        {
            _socket = new TcpClient(_authServer, _port);
            _stream = _socket.GetStream();
            GD.Print("Fase 1: Conectado al reino 3.3.5a. Enviando paquete de Login...");
            EnviarLoginChallenge();
        }
        catch (Exception e)
        {
            GD.PrintErr("Error de conexión: " + e.Message);
        }
    }

    private void EnviarLoginChallenge()
    {
        // Estructura simplificada del paquete de login (SMSG_AUTH_CHALLENGE para 3.3.5a)
        byte[] packet = new byte[4] { 0x00, 0x01, 0x02, 0x03 }; 
        _stream.Write(packet, 0, packet.Length);
    }

    // Fase 4: Enviar petición de hechizo (ej. Opcode SMSG_CAST_FAILED / CMSG_CAST_SPELL)
    public void EnviarCastSpell(int spellId, ulong targetGuid)
    {
        GD.Print($"Fase 4: Lanzando Hechizo ID {spellId} al objetivo {targetGuid}");
        // Aquí se serializa el Opcode de combate de la 3.3.5a
    }
}
