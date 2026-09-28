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
        // 'user://' es mapeado automáticamente por Godot a la carpeta de almacenamiento segura en Android y PC
        string rutaBase = ProjectSettings.GlobalizePath("user://");
        string rutaRealmlist = Path.Combine(rutaBase, "realmlist.wtf");

        // Si no existe, creamos uno por defecto para que el usuario pueda editarlo en su dispositivo
        if (!File.Exists(rutaRealmlist))
        {
            try
            {
                Directory.CreateDirectory(rutaBase);
                File.WriteAllText(rutaRealmlist, "set realmlist 127.0.0.1\n");
                GD.Print($"[Red] Archivo realmlist.wtf creado por defecto en: {rutaRealmlist}");
            }
            catch (Exception e) { GD.PrintErr("No se pudo crear realmlist base: " + e.Message); }
        }

        if (File.Exists(rutaRealmlist))
        {
            try
            {
                string[] lineas = File.ReadAllLines(rutaRealmlist);
                foreach (string linea in lineas)
                {
                    string limpia = linea.Trim();
                    if (limpia.ToLower().StartsWith("set realmlist"))
                    {
                        // Extraemos la IP o Dominio del servidor
                        _authServer = limpia.Substring(13).Trim();
                        GD.Print($"[Red] Realmlist cargado con éxito: {_authServer}");
                        break;
                    }
                }
            }
            catch (Exception e) { GD.PrintErr("Error al leer realmlist: " + e.Message); }
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
        
        // Estructura mínima simplificada del paquete de Login para WoW 3.3.5a (WotLK)
        byte[] packet = new byte[4 + userBytes.Length];
        
        packet[0] = 0x00; // Opcode: AUTH_LOGON_CHALLENGE
        packet[1] = 0x03; // Error/Status placeholder 
        packet[2] = (byte)(3 + userBytes.Length); // Tamaño del resto del paquete (indicador de longitud)
        packet[3] = (byte)userBytes.Length;       // Longitud exacta de la cadena del usuario

        Array.Copy(userBytes, 0, packet, 4, userBytes.Length);
        
        _stream.Write(packet, 0, packet.Length);
        GD.Print("[Red] Handshake inicial enviado al AuthServer WoW. Esperando respuesta binaria...");
    }

    private void EscucharServidor()
    {
        byte[] buffer = new byte[2048]; // Incrementamos el tamaño para paquetes SRP6 grandes
        while (_running && _stream != null)
        {
            try
            {
                int bytesRead = _stream.Read(buffer, 0, buffer.Length);
                if (bytesRead > 0)
                {
                    byte opcodeRespuesta = buffer[0];
                    
                    // IMPORTANTE: Procesamos la respuesta de forma diferida en el hilo principal de Godot
                    Callable.From(() => ProcesarOpcodeEnHiloPrincipal(opcodeRespuesta, buffer, bytesRead)).CallDeferred();
                }
            }
            catch 
            { 
                break; 
            }
        }
    }

    private void ProcesarOpcodeEnHiloPrincipal(byte opcode, byte[] datos, int tamano)
    {
        GD.Print($"[Red] Paquete procesado de forma segura en el Main Thread. Opcode: {opcode} ({tamano} bytes)");
        
        if (opcode == 0x00) // AUTH_LOGON_CHALLENGE Respuesta del Servidor
        {
            // Aquí inicia el cálculo matemático de las claves SRP6 (Generar los valores de B, g, N, s, etc.)
            GD.Print("[Red] Descomponiendo datos SRP6 del emulador para generar la clave de sesión...");
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
        // En Godot 4.x se usan constantes enteras para las notificaciones de cierre
        if (what == NotificationWMCloseRequest || what == 1006) // 1006 equivale al antiguo NotificationCrash
        {
            _running = false;
            _stream?.Close();
            _socket?.Close();
            
            // Es más seguro dejar que el hilo muera de forma natural al cerrar el stream que forzar un Abort
            if (_listenThread != null && _listenThread.IsAlive)
            {
                _listenThread.Join(500); 
            }
        }
    }
}
