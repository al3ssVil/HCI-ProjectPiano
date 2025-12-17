using UnityEngine;
using TMPro;
using System.Collections.Generic;

public class ScannedTargetsList : MonoBehaviour
{
    [Header("UI")]
    [SerializeField] private TMP_Text listText;

    [Header("Format")]
    [SerializeField] private string header = "Scanate:";
    [SerializeField] private string bullet = "•";
    [SerializeField] private bool preventDuplicates = true;

    private readonly List<string> items = new();

    public void Register(string displayName)
    {
        string name = Normalize(displayName);
        if (string.IsNullOrEmpty(name)) return;

        if (preventDuplicates)
        {
            if (!items.Contains(name)) items.Add(name);
        }
        else
        {
            items.Add(name);
        }
        UpdateUI();
    }

    public void RegisterFromObject(GameObject go)
    {
        if (!go) return;
        Register(go.name);
    }

    public void ClearAll()
    {
        items.Clear();
        UpdateUI();
    }

    private void UpdateUI()
    {
        if (!listText) return;
        if (items.Count == 0) { listText.text = $"{header} —"; return; }

        listText.text = $"{header} {string.Join($"  {bullet}  ", items)}";
    }

    private static string Normalize(string s)
    {
        return string.IsNullOrWhiteSpace(s) ? "" : s.Trim();
    }
}
