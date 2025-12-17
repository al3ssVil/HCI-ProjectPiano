using UnityEngine;
using TMPro;

public class PartituraLabel : MonoBehaviour
{
    [Header("Referințe (setează în Inspector)")]
    [SerializeField] private ButtonsColoring piano;  
    [SerializeField] private TMP_Text label;          

    [Header("Format")]
    [SerializeField] private bool includeLabel = true; 
    [SerializeField] private bool markCurrent = true;    
    [SerializeField] private string separator = " ";

    void Awake()
    {

        if (!piano)
        {
        #if UNITY_2023_1_OR_NEWER
            piano = Object.FindFirstObjectByType<ButtonsColoring>();
        #else
            piano = Object.FindObjectOfType<ButtonsColoring>();
        #endif
        }
    }

    void OnEnable()
    {
        if (!piano || !label) return;
        label.text = piano.GetPartituraString(includeLabel, markCurrent, separator);
        piano.OnPartituraChanged += HandlePartituraChanged;
    }

    void OnDisable()
    {
        if (piano != null) piano.OnPartituraChanged -= HandlePartituraChanged;
    }

    private void HandlePartituraChanged(string s)
    {
        if (label) label.text = s;  
    }

    public void RefreshNow()
    {
        if (piano && label)
            label.text = piano.GetPartituraString(includeLabel, markCurrent, separator);
    }

    public void ClearLabel()
    {
        if (label) label.text = includeLabel ? "Partitura: —" : "—";
    }
}
