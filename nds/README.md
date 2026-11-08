# Sky Dash – DSi Homebrew Skeleton

**by Sky Games**

---

## Wichtiger Hinweis / Important Notice

Dies ist **kein** vollständiges 1:1-Port von Web-Dashers / Geometry Dash.

Ein echter 1:1-Nachbau mit allen Features (Physik, Gamemodes, Level-Editor, Icon-Kit, Practice Mode, alle Levels usw.) ist auf der Nintendo DSi **nicht realistisch umsetzbar**, ohne das gesamte Spiel von Grund auf in nativem C/C++ neu zu schreiben. Das wäre ein mehrmonatiges bis jahrelanges Projekt für erfahrene Homebrew-Entwickler.

Dieses Repository enthält nur einen **minimalen, kompilierbaren Skeleton**:

- Titelbildschirm mit dem Namen **Sky Dash** / **Sky Games**
- Einfaches Icon
- Online- / WLAN-Features wurden bewusst **komplett weggelassen**
- Kurzes Audio-Sample (aus Stereo Madness konvertiert)
- Korrekte libnds / devkitPro Projektstruktur

Das Ergebnis ist eine lauffähige `.nds`, die auf echter Hardware und Emulatoren startet – aber **kein spielbares Geometry-Dash-ähnliches Spiel**.

---

## Was enthalten ist

```
sky-dash-dsi/
├── source/
│   └── main.c          ← Minimaler Titelbildschirm
├── graphics/
│   ├── icon.bmp        ← 32×32 Icon für das Menü / Banner
│   └── icon48.png
├── nitrofiles/
│   └── music/
│       └── stereo_madness_sample.wav   ← 12-Sekunden-Sample (22 kHz Mono)
├── Makefile
└── README.md
```

---

## Bauen der .nds

Voraussetzungen:
- [devkitPro](https://devkitpro.org/) mit devkitARM und libnds installiert
- `DEVKITARM` und `DEVKITPRO` korrekt gesetzt

```bash
cd sky-dash-dsi
make
```

Die fertige Datei heißt dann `sky-dash.nds`.

### GitHub Actions (Beispiel)

Du kannst dieses Repo auf GitHub legen und mit einer Workflow-Datei automatisch bauen lassen (devkitPro Docker-Image).

---

## Rebranding

- Original: Geometry Dash / RobTop
- Hier: **Sky Dash** / **Sky Games**

Alle Online-Funktionen (Level-Download, Account, API-Proxies usw.) sind entfernt bzw. nie implementiert worden.

---

## Nächste Schritte (falls jemand weitermachen will)

Um ein wirkliches Spiel daraus zu machen, müsste man:

1. Eigene Physik-Engine schreiben (oder anpassen)
2. GD-Level-Format parsen
3. Alle Sprites und Backgrounds stark runterskalieren und in DS-Formate bringen
4. Audio-Streaming über NitroFS implementieren
5. Dual-Screen-Rendering + Input optimieren
6. Menüs, Icon-Kit, Practice Mode usw. neu bauen

Das ist ein eigenständiges großes Homebrew-Projekt und hat mit dem vorliegenden Web-Code (Phaser.js) fast nichts mehr zu tun.

---

## Credits / Herkunft

Assets und Konzept inspiriert von dem Web-Dashers-Projekt und Geometry Dash.
Dieses Skeleton dient nur als Ausgangspunkt und Demonstration.

Viel Erfolg beim Weiterentwickeln!
