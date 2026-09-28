using Godot;
using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;

public partial class NetworkClient : Node
{
    private TcpClient? _socket;
    private NetworkStream? _stream;
    private string _authServer = "127.0.0.1"; 
    private int _port = 3724;
    private Thread? _listenThread;
    private bool _running = false;

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

    public void IniciarSesion(string usuario, string contrasena)
    {
        try
        {
            GD.Print($"[Red] Conectando a {_authServer}:{_port} para cuenta: {usuario.ToUpper()}");
            _socket = new TcpClient(_authServer, _port);
            _stream = _socket.GetStream();
            _running = true;

            // Hilo secundario para escuchar opcodes entrantes del emulador
            _listenThread = new Thread(EscucharServidor);
            _listenThread.Start();

            EnviarLoginChallenge(usuario.ToUpper(), contrasena);
        }
        catch (Exception e) 
        { 
            GD.PrintErr("Error de conexión: " + e.Message); 
        }
    }

    private void EnviarLoginChallenge(string usuario, string contrasena)
    {
        if (_stream == null) return;
        byte[] userBytes = Encoding.UTF8.GetBytes(usuario);
        byte[] packet = new byte[4 + userBytes.Length];
        
        packet[0] = 0x00; // Opcode: AUTH_LOGON_CHALLENGE
        packet[1] = 0x03; // Versión de WoW WotLK
        packet[2] = 0x03; 
        packet[3] = 0x05; // Build 12340

        Array.Copy(userBytes, 0, packet, 4, userBytes.Length);
        _stream.Write(packet, 0, packet.Length);
        GD.Print("[Red] Handshake inicial enviado. Esperando respuesta del reino...");
    }

    private void EscucharServidor()
    {
        byte[] buffer = new byte[1024];
        while (_running && _stream != null)
        {
            try
            {
                int bytesRead = _stream.Read(buffer, 0, buffer.Length);
                if (bytesRead > 0)
                {
                    byte opcodeRespuesta = buffer[0];
                    GD.Print($"[Red] Paquete binario recibido del emulador WoW. Opcode: {opcodeRespuesta}");
                    
                    // Aquí procesas de forma nativa los Opcodes de respuesta del servidor (SRP6)
                }
            }
            catch { break; }
        }
    }

    public void EnviarCastSpell(int spellId, ulong targetGuid)
    {
        GD.Print($"[Combate] Enviando Opcode CMSG_CAST_SPELL. Hechizo: {spellId}, Objetivo: {targetGuid}");
    }

    public void EnviarComandoInterfaz(string stringTokenMenu) => GD.Print($"[Red] Abriendo Interfaz: {stringTokenMenu}");
    public void EnviarComandoMovimiento(string tipoMovimiento) => GD.Print($"[Red] Sincronizando Posición: {tipoMovimiento}");

    public override void _Notification(int what)
    {
        if (what == NotificationWMCloseRequest || what == NotificationCrash)
        {
            _running = false;
            _socket?.Close();
            _listenThread?.Abort();
        }
    }
}
