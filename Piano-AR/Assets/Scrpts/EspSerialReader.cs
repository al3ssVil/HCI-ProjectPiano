using UnityEngine;
using System;
using System.IO.Ports;

[DisallowMultipleComponent]
public class EspSerialReader : MonoBehaviour
{
    [Header("Serial")]
    [SerializeField] string portName = "COM5";
    [SerializeField] int baudRate = 115200;
    [SerializeField] int readTimeoutMs = 25;
    [SerializeField] bool logLines = false;

    SerialPort serial;
    ButtonsColoring piano;

    void Awake()
    {
        piano = GetComponent<ButtonsColoring>();
        serial = new SerialPort(portName, baudRate)
        {
            NewLine = "\n",
            ReadTimeout = readTimeoutMs,
            DtrEnable = false,
            RtsEnable = false
        };
        try { serial.Open(); Debug.Log($"[ESP] Opening serial port {portName} at {baudRate} baud."); }
        catch (Exception e) { Debug.LogWarning($"[ESP] Open({portName}) failed: {e.Message}"); }
    }

    void Update()
    {
        if (serial == null || !serial.IsOpen || piano == null) return;

        try
        {
            while (serial.BytesToRead > 0)
            {
                string raw = serial.ReadLine();
                string note = Normalize(raw);
                if (string.IsNullOrEmpty(note)) continue;

                if (logLines) Debug.Log("[ESP] " + note);
                piano.SimulatePressByName(note); 
            }
        }
        catch (TimeoutException) { /* non-blocking */ }
        catch (Exception ex) { Debug.LogWarning($"[ESP] Read error: {ex.Message}"); }
    }

    void OnApplicationQuit()
    {
        try { if (serial != null && serial.IsOpen) serial.Close(); } catch { }
    }

    static string Normalize(string s)
    {
        if (string.IsNullOrWhiteSpace(s)) return "";
        return s.Trim().ToUpperInvariant().Replace(" ", "");
    }
}
