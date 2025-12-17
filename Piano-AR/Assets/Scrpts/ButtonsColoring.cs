using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

public class ButtonsColoring : MonoBehaviour
{
    [Header("Taste (12 sfere) – trage-le aici în ordine stânga→dreapta")]
    [SerializeField] private Renderer[] keyRenderers = new Renderer[12];

    [Header("Culori")]
    [SerializeField] private Color idleColor = new(0.75f, 0.75f, 0.75f, 1f); 
    [SerializeField] private Color targetColor = Color.green;
    [SerializeField] private Color wrongColor = Color.red;

    [Header("Partitură")]
    [Tooltip("Completezi ORI byName ORI byIndex; dacă ambele sunt setate, byIndex are prioritate.")]
    [SerializeField] private string[] songByName = new string[0]; 
    [SerializeField] private int[] songByIndex = new int[0];  

    [Header("Setări")]
    [SerializeField] private float wrongFlashSeconds = 2f;
    [SerializeField] private bool autoBindChildrenOnStart = true;

    private readonly Dictionary<string, int> noteToIndex = new()
    {
        {"C4",0},{"C#4",1},{"D4",2},{"D#4",3},{"E4",4},{"F4",5},
        {"F#4",6},{"G4",7},{"G#4",8},{"A4",9},{"A#4",10},{"B4",11},
    };

    private readonly List<int> song = new();
    private int step = 0;      
    private int expected = -1; 

    private bool IsFirstStart = false;
    void Awake()
    {
        if (autoBindChildrenOnStart &&
            (keyRenderers == null || keyRenderers.Length != 12 || keyRenderers.All(r => r == null)))
        {
            AutoBindFromChildren();
        }
    }

    void Start()
    {
        BuildSong();
        StartSong();
    }

    public void SimulatePressByName(string name)
    {
        var k = NormalizeNote(name);
        if (noteToIndex.TryGetValue(k, out var idx)) OnButtonDown(idx);
    }
    public void SimulatePress(int index) => OnButtonDown(index);

    public void OnButtonDown(int index)
    {
        if (index < 0 || index > 11) return;
        HandlePress(index);
    }

    [ContextMenu("Auto-bind keys from children")]
    public void AutoBindFromChildren()
    {
        var all = GetComponentsInChildren<Renderer>(includeInactive: true)
                  .Where(r => r && r.gameObject.name.StartsWith("Sphere"))
                  .OrderBy(r => r.gameObject.name, new NaturalSortComparer())
                  .Take(12)
                  .ToArray();
        if (all.Length == 12) keyRenderers = all;
        else Debug.LogWarning($"[ButtonsColoring] Găsite {all.Length}/12 sfere. Completează manual array-ul.");
    }

    void BuildSong()
    {
        song.Clear();

        if (songByIndex is { Length: > 0 })
        {
            foreach (var v in songByIndex) if (v >= 0 && v <= 11) song.Add(v);
        }
        else if (songByName is { Length: > 0 })
        {
            foreach (var s in songByName)
            {
                var k = NormalizeNote(s);
                if (noteToIndex.TryGetValue(k, out var idx)) song.Add(idx);
            }
        }

        if (song.Count == 0)
        {
            song.AddRange(new[] { 0, 2, 4, 5, 7, 9, 11 }); 
            Debug.LogWarning("[ButtonsColoring] Song gol. Am pus demo: 0,2,4,5,7,9,11");
        }
    }

    void StartSong()
    {
        step = 0;
        expected = song[0];
        NotifyPartitura();
        RepaintAll();
    }

    void Next()
    {
        step++;
        expected = (step < song.Count) ? song[step] : -1;
        RepaintAll();
        NotifyPartitura();
        if (expected < 0) Debug.Log("[ButtonsColoring] Melodie terminată.");
    }

    void HandlePress(int pressed)
    {
        if (expected < 0) return;

        if (pressed == expected)
        {
            SetKeyColor(expected, idleColor);
            Next();
        }
        else
        {
            StartCoroutine(FlashWrong(pressed));
        }
    }

    private static readonly string[] indexToNote =
    {
    "C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4"
    };

    public System.Action<string> OnPartituraChanged;

    public string GetPartituraString(bool includeLabel = true, bool markCurrent = true, string separator = " ")
    {
        if (song == null || song.Count == 0)
            return includeLabel ? "Partitura: —" : "—";

        var parts = new System.Collections.Generic.List<string>(song.Count);
        for (int i = 0; i < song.Count; i++)
        {
            int idx = song[i];
            string name = (idx >= 0 && idx < indexToNote.Length) ? indexToNote[idx] : "?";
            if (markCurrent && expected >= 0 && i == step) name = $"[{name}]";
            parts.Add(name);
        }

        string body = string.Join(separator, parts);
        return includeLabel ? $"Partitura: {body}" : body;
    }

    private void NotifyPartitura() => OnPartituraChanged?.Invoke(GetPartituraString());


    IEnumerator FlashWrong(int index)
    {
        if (!ValidKey(index)) yield break;

        int expectedAtStart = expected;
        SetKeyColor(index, wrongColor);

        if (expectedAtStart >= 0) SetKeyColor(expectedAtStart, targetColor);

        yield return new WaitForSeconds(wrongFlashSeconds);

        if (expected == expectedAtStart && index != expected)
            SetKeyColor(index, idleColor);
        else if (index == expected)
            SetKeyColor(index, targetColor);
    }

    void RepaintAll()
    {
        for (int i = 0; i < keyRenderers.Length; i++)
        {
            if (!ValidKey(i)) continue;
            SetKeyColor(i, (i == expected && expected >= 0) ? targetColor : idleColor);
        }
    }

    bool ValidKey(int i) =>
        keyRenderers != null && i >= 0 && i < keyRenderers.Length && keyRenderers[i];

    void SetKeyColor(int i, Color c)
    {
        if (!ValidKey(i)) return;
        var r = keyRenderers[i];
        var m = r.material; 
        if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", c);
        else if (m.HasProperty("_Color")) m.SetColor("_Color", c);
        else r.material.color = c;
    }

    static string NormalizeNote(string s)
    {
        if (string.IsNullOrWhiteSpace(s)) return "";
        return s.Trim().ToUpperInvariant().Replace(" ", ""); 
    }

    
    private bool acceptInput = true;

    public void OnTargetFoundEvent()
    {
        acceptInput = true;
        RestartSong();  
    }

    public void OnTargetLostEvent()
    {
        acceptInput = false;
        Clear();         
    }

    public void RestartSong()
    {
        BuildSong();     
        StartSong();     
        if (IsFirstStart == false)
        {
            IsFirstStart = true;
            Debug.Log("[ButtonsColoring] Start melodie.");
        }
        else
        {
            Debug.Log("[ButtonsColoring] Restart melodie");
        }
    }

    public void Clear()
    {
        expected = -1;   
        RepaintAll();
        NotifyPartitura();
    }

    class NaturalSortComparer : IComparer<string>
    {
        public int Compare(string a, string b)
        {
            int d = a.Length.CompareTo(b.Length);
            return d != 0 ? d : string.CompareOrdinal(a, b);
        }
    }
}
