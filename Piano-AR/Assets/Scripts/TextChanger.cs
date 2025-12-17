using UnityEngine;
using UnityEngine.UI;   
using TMPro;
using System.Collections.Generic;

public class TextChanger : MonoBehaviour
{
    [Header("UI")]
    [SerializeField] private TMP_Text nameText;   
    [SerializeField] private TMP_Text descText;  
    [SerializeField] private Button detailButton; 
    [SerializeField] private TMP_Text detailButtonLabel; 

    [Header("Fallback după Tag (opțional)")]
    [SerializeField] private string nameTextTag = "ComponentText";
    [SerializeField] private string descTextTag = "ComponentDescription";

    private enum ComponentType { None, ESP32, LCD, Pot, Buttons, Arduino }
    private ComponentType current = ComponentType.None;

    private bool showUsage = false;
    private bool userOverride = false;

    private readonly Dictionary<ComponentType, string> descGeneral = new()
    {
        { ComponentType.ESP32,   "ESP32: microcontroler dual-core cu Wi-Fi, perfect pentru proiecte IoT/AR." },
        { ComponentType.LCD,     "LCD 16x2: afișaj text controlabil pe 4/8 biți sau I2C pentru mesaje simple." },
        { ComponentType.Pot,     "Potențiometru: divizor de tensiune variabil, bun pentru reglaje analogice." },
        { ComponentType.Buttons, "Butoane: intrări digitale pentru acțiuni on/off sau meniuri." },
        { ComponentType.Arduino, "Arduino: plăcuță de dezvoltare ușor de utilizat pentru diverse proiecte." },
    };

    private readonly Dictionary<ComponentType, string> descUsage = new()
    {
        { ComponentType.ESP32,   "În pian: ESP32 controleaz logica, citește apăsarile de butoane." },
        { ComponentType.LCD,     "În pian: LCD-ul afișează nota curentă, si frecvența respectivei note muzicale." },
        { ComponentType.Pot,     "În pian: potențiometrul reglează frecventa notelor muzicale  apăsate." },
        { ComponentType.Buttons, "În pian: butoanele reprezintă cele 12 note muzicale ale pianului nostru." },
        { ComponentType.Arduino, "În pian: Arduino a fost predecesorul lui ESP32, controlând logica inițială." },
    };

    void Awake()
    {
        if (!nameText)
        {
            var go = GameObject.FindGameObjectWithTag(nameTextTag);
            if (go) nameText = go.GetComponent<TMP_Text>();
        }
        if (!descText)
        {
            var go = GameObject.FindGameObjectWithTag(descTextTag);
            if (go) descText = go.GetComponent<TMP_Text>();
        }
    }

    void Start()
    {
        if (detailButton)
        {
            detailButton.onClick.RemoveAllListeners();
            detailButton.onClick.AddListener(ShowDetail);
        }
        UpdateButtonLabel();
    }

    private void SetName(string v) { if (nameText) nameText.text = v; }
    private void SetDesc(string v) { if (descText) descText.text = v; }

    private void ApplyCurrentText()
    {
        if (current == ComponentType.None) { SetDesc(""); return; }
        var map = showUsage ? descUsage : descGeneral;
        if (map.TryGetValue(current, out var d)) SetDesc(d);
        UpdateButtonLabel();
    }

    private void UpdateButtonLabel()
    {
        if (detailButtonLabel)
            detailButtonLabel.text = showUsage ? "Arată general" : "Arată utilizare";
    }

    private void SetComponent(ComponentType t, string displayName)
    {
        bool newTarget = (current != t);
        current = t;
        SetName(displayName);

        if (newTarget)
        {
            showUsage = false;
            userOverride = false;

            if (detailButton)
            {
                detailButton.onClick.RemoveAllListeners();
                detailButton.onClick.AddListener(ShowDetail);
            }
        }

        if (!userOverride) ApplyCurrentText();
    }

    public void ShowESP32() => SetComponent(ComponentType.ESP32, "ESP32");
    public void ShowLCD() => SetComponent(ComponentType.LCD, "LCD");
    public void ShowPot() => SetComponent(ComponentType.Pot, "Potențiometru");
    public void ShowButtons() => SetComponent(ComponentType.Buttons, "Butoane");
    public void ShowArduino() => SetComponent(ComponentType.Arduino, "Arduino");

    public void ShowDetail()
    {
        if (current == ComponentType.None) return;
        showUsage = !showUsage;  
        userOverride = true;    
        ApplyCurrentText();
    }
    public void ToggleDescription() => ShowDetail();

    public void ClearText()
    {
        SetName("");
        SetDesc("");
        current = ComponentType.None;
        showUsage = false;
        userOverride = false;
        UpdateButtonLabel();
    }
}
