# Sky Dash – Nintendo DS / DSi

Dies ist der DS-Port der Sky-Dash-Level aus dem Browser-Projekt. Die Spielphysik
ist auf dieselben 240-Hz-Schritte, Bewegungswerte und Kollisionen portiert.

## `.nds` direkt auf GitHub bauen

1. Neues GitHub-Repository erstellen.
2. **Den gesamten Inhalt dieses Ordners** in das Repository hochladen – auch den
   versteckten Ordner `.github/workflows`.
3. In GitHub **Actions → Build Sky Dash DS** öffnen oder eine Änderung pushen.
4. Beim erfolgreichen Lauf unter **Artifacts → Sky-Dash-NDS** die Datei
   `skydash.nds` herunterladen.

Lokal kann die ROM mit devkitPro gebaut werden: `sh build_levels.sh` und danach
`make` ausführen. Weitere Hinweise stehen in `LIESMICH.txt`.

## DSi

Die Ausgabe ist eine normale `.nds`-Homebrew-ROM. Auf einem DSi läuft sie im
DS-Kompatibilitätsmodus, nicht als native DSiWare-/TWL-Anwendung.

## Hardwarebedingte Unterschiede

Die Simulation und Leveldaten sind an die Browser-Version angeglichen. Grafik
und Audio müssen jedoch auf DS-Auflösung, 15-Bit-Farbe und die DS-Soundhardware
umgesetzt werden. Online-Modus, Übungsmodus, Icon-Garage und dauerhaftes
Speichern sind in diesem DS-Port nicht enthalten.
