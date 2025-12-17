using UnityEngine;
using UnityEngine.UI;
using TMPro;
using System.Collections;
using System.Collections.Generic;

[DisallowMultipleComponent]
public class Objects3DChanger : MonoBehaviour
{
    public static Objects3DChanger Instance { get; private set; }

    [Header("UI (leagă-le în Inspector!)")]
    [SerializeField] private TMP_Text targetText;
    [SerializeField] private TMP_Text infoText;
    [SerializeField] private Button infoButton;

    private enum Obj { None, LCD, Piano, ESP32, Arduino, Breadboard }
    private Obj current = Obj.None;
    private bool infoShown = false;
    private Coroutine lostCo;

    private readonly Dictionary<Obj, string> info = new()
    {
        { Obj.LCD,        "LCD 16x2 I2C.\nAfișează nota/frecvența." },
        { Obj.Piano,      "Pian cu 12 butoane.\nNotele muzicale." },
        { Obj.ESP32,      "Plăcuța cu Wi-Fi/BLE.\nCoordonează logica." },
        { Obj.Arduino,    "Placă de prototipare.\nVersiunea inițială." },
        { Obj.Breadboard, "Placă + 12 butoane.\nIndici 0–11 pentru note." },
    };

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Debug.LogWarning($"[Objects3DChanger] Duplicat pe {gameObject.name}, îl dezactivez. Instanța activă: {Instance.gameObject.name}");
            enabled = false; return;
        }
        Instance = this;
    }

    void Start()
    {
        if (!targetText || !infoText || !infoButton)
            Debug.LogError("Leagă targetText, infoText și infoButton în Inspector (aceeași instanță)!");
        infoButton.onClick.RemoveAllListeners();
        infoButton.onClick.AddListener(ShowInfoForCurrent);
        RefreshButton();
    }

    private void RefreshButton() => infoButton.interactable = (current != Obj.None);

    private void Set3D(Obj type, string displayName)
    {
        if (lostCo != null) { StopCoroutine(lostCo); lostCo = null; } 
        current = type;
        infoShown = false;
        if (targetText) targetText.text = $"3D: {displayName}";
        if (infoText) infoText.text = "";
        RefreshButton();
        Debug.Log($"[Objects3DChanger#{GetInstanceID()}:{gameObject.name}] Current set to {current}");
    }

    public void Show3D_LCD() => Set3D(Obj.LCD, "LCD");
    public void Show3D_LCD360() => Set3D(Obj.LCD, "LCD 360");
    public void Show3D_Piano() => Set3D(Obj.Piano, "Piano");
    public void Show3D_ESP32() => Set3D(Obj.ESP32, "ESP32");
    public void Show3D_Arduino() => Set3D(Obj.Arduino, "Arduino");
    public void Show3D_Breadboard() => Set3D(Obj.Breadboard, "Breadboard");

    public void ShowInfoForCurrent()
    {
        Debug.Log($"[Objects3DChanger#{GetInstanceID()}] Button clicked, current={current}, shown={infoShown}");
        if (current == Obj.None || !infoText) return;

        if (!infoShown)
        {
            if (info.TryGetValue(current, out var txt)) infoText.text = txt;
            infoShown = true;
        }
        else
        {
            infoText.text = "";
            infoShown = false;
        }
    }

    public void Clear3D()
    {
        if (lostCo != null) StopCoroutine(lostCo);
        lostCo = StartCoroutine(ClearAfterDelay(0.25f));
    }
    private IEnumerator ClearAfterDelay(float delay)
    {
        yield return new WaitForSeconds(delay);
        current = Obj.None;
        infoShown = false;
        if (targetText) targetText.text = "";
        if (infoText) infoText.text = "";
        RefreshButton();
        Debug.Log($"[Objects3DChanger#{GetInstanceID()}] Cleared");
    }
}
