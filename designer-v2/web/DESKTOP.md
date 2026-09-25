# Windows desktop shell

The v0.2.0 Simple Mode is wrapped with Tauri 2 so the user can run it as a normal Windows application instead of managing a browser or Node manually.

Local developer commands:

```powershell
npm install
npm run desktop:dev
npm run desktop:build
```

The GitHub workflow builds a Windows executable with `tauri build --no-bundle` and uploads it as an artifact.

This desktop shell and the web UI use the same project model and renderer. No separate desktop renderer is introduced.
