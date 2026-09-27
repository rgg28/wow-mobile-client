using Godot;
using System;
using System.IO;
using System.Net.Sockets;

public partial class NetworkClient : Node
{
    private TcpClient _socket;
    private NetworkStream _stream;
    
    // IP de respaldo por si no encuentra el archivo realmlist.wtf
    private string _authServer = "127.0.0.1"; 
    private int _port = 3724;

    // Ruta de configuración en Android (misma carpeta de tus assets)
    private string _rutaConfigAndroid = "user://WoWAssets/realmlist.wtf";

    public override void _Ready()
    {
        CargarRealmlist();
        ConectarAlServidor();
    }

    private void CargarRealmlist()
    {
        // Determinar la ruta según la plataforma (Android o PC)
        string rutaFinal = OS.GetName() == "Android" 
            ? ProjectSettings.GlobalizePath(_rutaConfigAndroid) 
            : "./WoWAssets/realmlist.wtf";

        if (File.Exists(rutaFinal))
        {
            try
            {
                // Leer las líneas del archivo (ej: "set realmlist ://servidor.com")
                string[] lineas = File.ReadAllLines(rutaFinal);
                foreach (string linea in lineas)
                {
                    string limpia = linea.Trim().ToLower();
                    if (limpia.StartsWith("set realmlist"))
                    {
                        // Extraer solo la IP o dominio del servidor
                        string ipDetectada = linea.Replace("set realmlist", "", StringComparison.OrdinalIgnoreCase).Trim();
                        if (!string.IsNullOrEmpty(ipDetectada))
                        {
                            _authServer = ipDetectada;
                            GD.Print($"[Red] Realmlist cargado con éxito desde archivo: {_authServer}");
                            return;
                        }
                    }
                }
            }
            catch (Exception e)
            {
                GD.PrintErr($"[Red] Error al leer realmlist.wtf: {e.Message}");
            }
        }
        
        GD.Print($"[Red] No se encontró realmlist.wtf o está corrupto. Usando IP por defecto: {_authServer}");
    }

    public void ConectarAlServidor()
    {
        try
        {
            GD.Print($"[Red] Intentando conectar a {_authServer}:{_port}...");
            _socket = new TcpClient(_authServer, _port);
            _stream = _socket.GetStream();
            GD.Print("Fase 1: ¡Conectado con éxito! Enviando paquete de Login...");
            EnviarLoginChallenge();
        }
        catch (Exception e)
        {
            GD.PrintErr("[Red] Error de conexión: " + e.Message);
        }
    }

    private void EnviarLoginChallenge()
    {
        byte[] packet = new byte[] { 0x00, 0x01, 0x02, 0x03 }; 
        _stream.Write(packet, 0, packet.Length);
    }

    public void EnviarCastSpell(int spellId, ulong targetGuid)
    {
        if (_stream != null && _socket.Connected)
        {
            GD.Print($"Fase 4: Enviando hechizo ID {spellId} al objetivo {targetGuid}");
            // Aquí se empaqueta el Opcode de combate real de la 3.3.5a
        }
    }
}

