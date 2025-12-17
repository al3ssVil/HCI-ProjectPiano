using UnityEngine;

public class ScanEventRelay : MonoBehaviour
{
    [SerializeField] private ScannedTargetsList list;   
    [SerializeField] private string displayName;      
    [SerializeField] private bool fallbackToGOName = true;

    public void NotifyFound()
    {
        if (!list) return;
        string nameToAdd = !string.IsNullOrWhiteSpace(displayName)
            ? displayName
            : (fallbackToGOName ? gameObject.name : "");
        list.Register(nameToAdd);
    }
}
