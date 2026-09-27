using Godot;
using System;
using System.IO;
using System.Net.Sockets;

public partial class NetworkClient : Node
{
    private TcpClient _socket;
    private NetworkStream _stream;
    private string _authServer = "127.0.0.1"; 
    private int _port = 3724;

    public override void _Ready()
    {
        // En Android lee la carpeta pública Documents, en PC usa la raíz local
        string rutaBase = OS.GetName() == "Android" 
            ? "/storage/emulated/0/Documents/WoW335Android/" 
            : "./WoWAssets/";

        string rutaRealmlist = Path.Combine(rutaBase, "realmlist.wtf");

        if (File.Exists(rutaRealmlist))
        {
            try
            {
                string[] lineas = File.ReadAllLines(rutaRealmlist);
                foreach (string linea in lineas)
                {
                    string limpia = linea.Trim().ToLower();
                    if (limpia.StartsWith("set realmlist"))
                    {
                        _authServer = linea.Replace("set realmlist", "", StringComparison.OrdinalIgnoreCase).Trim();
                        GD.Print($"[Red] Realmlist cargado con éxito: {_authServer}");
                        break;
                    }
                }
            }
            catch (Exception e) { GD.PrintErr("Error realmlist: " + e.Message); }
        }

        ConectarAlServidor();
    }

    public void ConectarAlServidor()
    {
        try
        {
            _socket = new TcpClient(_authServer, _port);
            _stream = _socket.GetStream();
            GD.Print("Fase 1: Conectado a la 3.3.5a. Enviando handshake...");
            EnviarLoginChallenge();
        }
        catch (Exception e) { GD.PrintErr("Error de conexión: " + e.Message); }
    }

    private void EnviarLoginChallenge()
    {
        byte[] packet = new byte[] { 0x00, 0x01, 0x02, 0x03 }; 
        _stream.Write(packet, 0, packet.Length);
    }

    public void EnviarCastSpell(int spellId, ulong targetGuid)
    {
        GD.Print($"Fase 4: Lanzando Hechizo ID {spellId} al objetivo {targetGuid}");
    }
}


