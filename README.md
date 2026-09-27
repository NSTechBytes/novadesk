<p align="center">
  <img src="https://github.com/Official-Novadesk/novadesk-assets/blob/master/app-logo/logo-text.png?raw=true" alt="Novadesk Logo"/>
  <br>
  <a href="https://novadesk.pages.dev">
    <img src="https://img.shields.io/badge/Website-Visit-3b82f6?style=for-the-badge&logo=google-chrome&logoColor=white" alt="Website"/>
  </a>
  <a href="https://novadesk-docs.pages.dev">
    <img src="https://img.shields.io/badge/Documentation-Read-f59e0b?style=for-the-badge&logo=gitbook&logoColor=white" alt="Documentation"/>
  </a>
  <a href="https://github.com/Official-Novadesk/novadesk/releases/latest">
    <img src="https://img.shields.io/badge/Download-Latest-10b981?style=for-the-badge&logo=github&logoColor=white" alt="Download"/>
  </a>
  <br>
  <img src="https://img.shields.io/github/license/Official-Novadesk/novadesk?style=for-the-badge" alt="License"/>
  <img src="https://img.shields.io/github/v/release/Official-Novadesk/novadesk?style=for-the-badge" alt="Release"/>
  <img src="https://img.shields.io/github/stars/Official-Novadesk/novadesk?style=for-the-badge" alt="Stars"/>
  <img src="https://img.shields.io/github/issues/Official-Novadesk/novadesk?style=for-the-badge" alt="Issues"/>
</p>

Novadesk is a open source powerful desktop widget platform built with C++. It allows users to create system monitors, custom interfaces, and desktop enhancements using familiar JavaScript syntax.

## 🔗 Resources

- 🌐 [Official Website](https://novadesk.pages.dev) - Learn more about Novadesk features and updates.
- 📚 [Documentation](https://novadesk-docs.pages.dev) - Detailed guides on creating widgets and using the platform.

## ❤️ Support / Donate

If you enjoy using Novadesk and want to support its development, consider becoming a patron:

- ☕ [Patreon](https://www.patreon.com/c/officialnovadesk) - Support the project's growth and get exclusive updates.

## Novadesk Ecosystem

Full diagrams (all `src/apps` apps, `novadesk/` layers, render class tree, and data flow): **[src/apps/ARCHITECTURE.md](src/apps/ARCHITECTURE.md)**

```mermaid
flowchart TB
  subgraph src_apps ["Novadesk Ecosystem and Tools"]
    Novadesk["novadesk/<br/>Desktop Runtime EXE"]
    Nwm["nwm/<br/>CLI: init, run, build"]
    Manage["manage_novadesk/<br/>Widget manager GUI"]
    Ndpkg["ndpkg_installer/<br/>.ndpkg package installer"]
    Stub["installer_stub/<br/>Standalone SFX bootstrap"]
    Nsis["installer/<br/>NSIS Full Setup"]
    Assets["assets/images/<br/>Shared UI Assets"]
  end

  Dev["Developer"] -->|"init / run / build"| Nwm
  User["End User"] -->|"manage widgets"| Manage
  User -->|"launch and interact"| Novadesk
  User -->|"open .ndpkg"| Ndpkg

  Nwm -->|"standalone EXE"| Stub
  Nwm -->|"package"| Ndpkg
  Nwm -->|"run widget"| Novadesk
  Manage -->|"load / unload"| Novadesk
  Ndpkg -->|"install to Widgets/"| Novadesk
  Stub -->|"extract to Widgets/"| Novadesk
  Nsis -->|"platform install"| Novadesk
  Nsis -->|"platform install"| Manage

  subgraph novadesk_layers ["novadesk/ internals"]
    JS["scripting/quickjs<br/>JSEngine, Modular PropertyParsers, Modules"]
    Domain["domain/<br/>Widget, DesktopManager, Animation, DropTarget, Popups"]
    Render["render/<br/>Elements, Shapes, FlexLayoutEngine, Direct2D"]
    Shared["shared/<br/>Settings, Logging, Utils, ZipUtils, ColorUtil"]
  end

  Novadesk --> JS
  JS --> Domain
  Domain --> Render
  Domain --> Shared
  Render --> Shared
```
