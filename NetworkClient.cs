using Godot;
using System;
using System.IO;
using System.Net.Sockets;
using System.Text;

public partial class NetworkClient : Node
{
    private TcpClient _socket;
    private NetworkStream _stream;
    private string _authServer = "127.0.0.1"; 
    private int _port = 3724;

    public override void _Ready()
    {
        CargarRealmlist();
    }

    private void CargarRealmlist()
    {
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
                        GD.Print($"[Red] Realmlist cargado: {_authServer}");
                        break;
                    }
                }
            }
            catch (Exception e) { GD.PrintErr("Error realmlist: " + e.Message); }
        }
    }

    // Método que llama la UI cuando el usuario presiona "Conectar"
    public void IniciarSesion(string usuario, string contrasena)
    {
        try
        {
            GD.Print($"[Red] Conectando a {_authServer}:{_port} con el usuario: {usuario.ToUpper()}");
            _socket = new TcpClient(_authServer, _port);
            _stream = _socket.GetStream();
            
            // Fase 1: Enviar paquete de autenticación original de la 3.3.5a (Auth Challenge)
            EnviarLoginChallenge(usuario.ToUpper(), contrasena);
        }
        catch (Exception e) 
        { 
            GD.PrintErr("Error de conexión: " + e.Message); 
        }
    }

    private void EnviarLoginChallenge(string usuario, string contrasena)
    {
        // En un cliente de ingeniería inversa completo, aquí se calcula el algoritmo SRP6 (Criptografía de WoW)
        // Por ahora, estructuramos el paquete inicial con el nombre de usuario para el handshake
        byte[] userBytes = Encoding.UTF8.GetBytes(usuario);
        byte[] packet = new byte[4 + userBytes.Length];
        
        packet[0] = 0x00; // Opcode: AUTH_LOGON_CHALLENGE
        packet[1] = 0x03; // Versión de WoW (3)
        packet[2] = 0x03; // Sub-versión (3)
        packet[3] = 0x05; // Build (5 -> 12340 para la 3.3.5a)

        Array.Copy(userBytes, 0, packet, 4, userBytes.Length);
        
        _stream.Write(packet, 0, packet.Length);
        GD.Print("[Red] Paquete de Auth Challenge enviado al servidor.");
    }

    public void EnviarCastSpell(int spellId, ulong targetGuid)
    {
        GD.Print($"Fase 4: Lanzando Hechizo ID {spellId} al objetivo {targetGuid}");
    }

    public void EnviarComandoInterfaz(string stringTokenMenu) => GD.Print($"[Red] UI: {stringTokenMenu}");
    public void EnviarComandoMovimiento(string tipoMovimiento) => GD.Print($"[Red] Movimiento: {tipoMovimiento}");
}
