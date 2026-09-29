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
    private volatile bool _running = false;

    private bool _initialized = false;

    public override void _Ready()
    {
        GD.Print("[Red] NetworkClient iniciado.");

        /*
         * IMPORTANTE:
         *
         * No conectamos al servidor aquí.
         * No abrimos sockets aquí.
         * No creamos hilos aquí.
         *
         * El cliente solamente prepara la configuración.
         */
        try
        {
            CargarRealmlist();

            _initialized = true;

            GD.Print("[Red] NetworkClient listo.");
        }
        catch (Exception ex)
        {
            GD.PrintErr("[Red] Error inicializando NetworkClient:");
            GD.PrintErr(ex.ToString());

            _initialized = false;
        }
    }

    private void CargarRealmlist()
    {
        string rutaBase = ProjectSettings.GlobalizePath("user://");

        if (string.IsNullOrEmpty(rutaBase))
        {
            GD.PrintErr("[Red] No se pudo determinar user://");
            return;
        }

        string rutaRealmlist = Path.Combine(
            rutaBase,
            "realmlist.wtf"
        );

        GD.Print($"[Red] Configuración: {rutaRealmlist}");

        if (!File.Exists(rutaRealmlist))
        {
            try
            {
                Directory.CreateDirectory(rutaBase);

                File.WriteAllText(
                    rutaRealmlist,
                    "set realmlist 127.0.0.1\n"
                );

                GD.Print(
                    "[Red] realmlist.wtf creado correctamente."
                );
            }
            catch (Exception ex)
            {
                GD.PrintErr(
                    "[Red] No se pudo crear realmlist.wtf:"
                );

                GD.PrintErr(ex.Message);

                return;
            }
        }

        try
        {
            string[] lineas =
                File.ReadAllLines(rutaRealmlist);

            foreach (string linea in lineas)
            {
                string limpia = linea.Trim();

                if (limpia.StartsWith(
                    "set realmlist",
                    StringComparison.OrdinalIgnoreCase))
                {
                    string servidor =
                        limpia.Substring(13).Trim();

                    if (!string.IsNullOrWhiteSpace(servidor))
                    {
                        _authServer = servidor;

                        GD.Print(
                            $"[Red] Realmlist: {_authServer}"
                        );
                    }

                    break;
                }
            }
        }
        catch (Exception ex)
        {
            GD.PrintErr(
                "[Red] Error leyendo realmlist:"
            );

            GD.PrintErr(ex.Message);
        }
    }

    public void IniciarSesion(
        string usuario,
        string contrasena)
    {
        if (!_initialized)
        {
            GD.PrintErr(
                "[Red] NetworkClient no está inicializado."
            );

            return;
        }

        if (string.IsNullOrWhiteSpace(usuario))
        {
            GD.PrintErr(
                "[Red] Usuario vacío."
            );

            return;
        }

        try
        {
            CerrarConexion();

            GD.Print(
                $"[Red] Conectando a {_authServer}:{_port}"
            );

            _socket = new TcpClient();

            _socket.Connect(
                _authServer,
                _port
            );

            _stream = _socket.GetStream();

            _running = true;

            _listenThread =
                new Thread(EscucharServidor);

            _listenThread.IsBackground = true;

            _listenThread.Start();

            EnviarLoginChallenge(
                usuario.ToUpperInvariant(),
                contrasena
            );
        }
        catch (Exception ex)
        {
            GD.PrintErr(
                "[Red] Error de conexión:"
            );

            GD.PrintErr(ex.Message);

            CerrarConexion();
        }
    }

    private void EnviarLoginChallenge(
        string usuario,
        string contrasena)
    {
        if (_stream == null)
        {
            GD.PrintErr(
                "[Red] Stream no disponible."
            );

            return;
        }

        try
        {
            byte[] userBytes =
                Encoding.UTF8.GetBytes(usuario);

            byte[] packet =
                new byte[4 + userBytes.Length];

            packet[0] = 0x00;
            packet[1] = 0x03;

            packet[2] =
                (byte)(3 + userBytes.Length);

            packet[3] =
                (byte)userBytes.Length;

            Array.Copy(
                userBytes,
                0,
                packet,
                4,
                userBytes.Length
            );

            _stream.Write(
                packet,
                0,
                packet.Length
            );

            GD.Print(
                "[Red] Handshake enviado."
            );
        }
        catch (Exception ex)
        {
            GD.PrintErr(
                "[Red] Error enviando login:"
            );

            GD.PrintErr(ex.Message);
        }
    }

    private void EscucharServidor()
    {
        byte[] buffer = new byte[4096];

        while (_running)
        {
            try
            {
                if (_stream == null)
                    break;

                int bytesRead =
                    _stream.Read(
                        buffer,
                        0,
                        buffer.Length
                    );

                if (bytesRead <= 0)
                    break;

                byte[] datos =
                    new byte[bytesRead];

                Array.Copy(
                    buffer,
                    datos,
                    bytesRead
                );

                byte opcode =
                    datos[0];

                Callable.From(
                    () =>
                        ProcesarOpcodeEnHiloPrincipal(
                            opcode,
                            datos,
                            datos.Length
                        )
                ).CallDeferred();
            }
            catch (Exception ex)
            {
                if (_running)
                {
                    GD.PrintErr(
                        "[Red] Error leyendo servidor:"
                    );

                    GD.PrintErr(ex.Message);
                }

                break;
            }
        }
    }

    private void ProcesarOpcodeEnHiloPrincipal(
        byte opcode,
        byte[] datos,
        int tamano)
    {
        GD.Print(
            $"[Red] Opcode recibido: {opcode} ({tamano} bytes)"
        );

        if (opcode == 0x00)
        {
            GD.Print(
                "[Red] AUTH_LOGON_CHALLENGE recibido."
            );
        }
    }

    public void EnviarCastSpell(
        int spellId,
        ulong targetGuid)
    {
        GD.Print(
            $"[Combate] Spell={spellId} Target={targetGuid}"
        );
    }

    public void EnviarComandoInterfaz(
        string stringTokenMenu)
    {
        GD.Print(
            $"[Red] Interfaz: {stringTokenMenu}"
        );
    }

    public void EnviarComandoMovimiento(
        string tipoMovimiento)
    {
        GD.Print(
            $"[Red] Movimiento: {tipoMovimiento}"
        );
    }

    public void CerrarConexion()
    {
        _running = false;

        try
        {
            _stream?.Close();
        }
        catch
        {
        }

        try
        {
            _socket?.Close();
        }
        catch
        {
        }

        _stream = null;
        _socket = null;
    }

    public override void _Notification(int what)
    {
        if (what == NotificationWMCloseRequest)
        {
            CerrarConexion();
        }
    }
}
