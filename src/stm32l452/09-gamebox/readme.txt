Titel:    Gamebox mit SAMSUNG SLM1608 Display
Autor:    (c) 2004-2006 by Malte Marwedel
Datum:    2006-11-03
Version:  1.00 (Final 1)
Zweck:    Spiele auf einem Grafikdisplay
Software: GCC-AVR / GCC
Hardware: LED Panel, ATMEGA32 oder ähnlichen mit 8MHZ.
          Einzelne Module müssten auch auf einem ATMEGA16 laufen.
          "modul_pxxo" und "modul_psnake" könnten jedoch mangels RAM
          möglicherweise gar nicht oder nur fehlerhaft laufen.
          ODER:
          PC mit 500MHZ und 800x600 @ 256 Farben 3D beschleunigte Grafik
Bemerkung:Der Quellcode steht unter der GPL. Der Lizenztext kann in der Datei
          gpl-license.txt gefunden werden.
          Ausgenommen hiervon sind die Dateien "avr/Makefile" und
          "i386/Makefile". Sie entstammt dem Programm "mfile" und für die Datei
          gelten entsprechende andere Bedingungen für die Verwendung.
Wer Fragen oder Anregungen zu dem Programm hat, kann an
           m.marwedel <AT> onlinehome.de mailen.
          Mehr über Elektronik und AVRs gibt es auf meiner Homepage:
           http://www.marwedels.de/malte/
Code Größe:ATMEGA32: 29710 Byte (getestet, alle Module)
Compiler Optionen:
          AVR: -Os -ffast-math -fweb -Winline
          PC: -Os -ffast-math -Winline --param max-inline-insns-single=100
Verwendete Software zum compilieren:
      AVR:
          avr-gcc  Version: 4.7.2
          avr-libc Version: 1.8.0
       PC:
          gcc Version: 4.7.2
          GLUT

-------------------------Hinweise zur AVR und PC Version -----------------------
Die Software wurde ursprünglich nur zum Ausführen auf einem Mikrocontroller
geschrieben. Die Möglichkeit die Software auch auf dem PC zu testen kam erst
später hinzu. Dabei habe ich versucht den Code möglichst wenig zu ändern.
Im primärem Verzeichnis befindet sich der vom AVR und PC gemeinsam benutzte
Code. Das Verzeichnis /avr enthält den AVR spezifischen und /i386 den PC
spezifischen Code. Der Code ist also optimiert für einen Mikrocontroller, sprich
eine Endlosschleife erledigt die meisten Berechnungen und Zeitsteuerungen werden
sowohl über Delay Schleifen, als auch über einen 16Bit Timer geregelt. Der
Interrupt eines 8Bit Timers sorgt für die Grafikausgabe und ein anderer
Interrupt eines zweiten 8Bit Timers sorgt für die Joystick Eingabe.
Auf dem PC ist nun die Grafikausgabe und Tasteneingabe grundlegend anders, so
dass hier Anstelle der beiden 8Bit Timer Funktionen der GLUT Libray diese
Aufgabe übernehmen. Die Hauproutine (Endlosschleife) wird als extra Thread
gestartet und verbraucht somit soviel CPU Rechenzeit wie möglich. Dies bedeutet,
solange das Programm auf dem PC läuft liegt die CPU Auslastung kontinuierlich
bei 100%. Dank des Betriebssystems bekommen aber natürlich auch andere Prozesse
Rechenzeit ab und der Rechner hängt nicht :-)
Die Delayschleife musste neu geschrieben werden und erledigt ihre Aufgabe durch
Auslesen der Systemzeit und Vergleich mit der gewünschten Delay Zeit.
Der ebenfalls zum Timing herangezogene 16Bit Timer wird simuliert indem Mithilfe
der GLUT Libray periodisch eine Funktion ausgeführt wird welche die 16Bit Timer
Variable entsprechend erhöht. Leider ist das periodische Ausführen dieser
Funktion nur mit einer relativ niedrigen Frequenz möglich, so dass der Timer
immer um mehrer Zählschritte auf einmal erhöht werden muss. Auch ist die Dauer
zwischen dem Ausführen der Funktion nicht immer gleich groß, so dass die
Zählschritte mal größer und mal kleiner ausfallen. Dies liegt vorallem daran,
dass ein CPU Kern nur einen Thread zur Zeit bearbeiten kann und der Wechsel
zwischen den verschiedenen Threads seltener erfolgt als die Funktion zum Erhöhen
des 16Bit Zählers aufgerufen werden müsste.
Das Resultat davon äußert sich darin dass die Software etwas stockend und träge
wirkt. Auf dem AVR sind die Bewegungen deutlich gleichmäßiger und das Spielen
der Spiele macht definitiv mehr Spaß!
Das Laufen auf dem PC dient daher eher
1. als Experiment in wie weit Code von der einen Plattform auch auf der andren
läuft
2. Als Plattform um Fehler schneller zu finden - einfachere Fehlerausgabe, kein
umständlicher Upload in den Mikrocontroller
und 3. um die Möglichkeiten der Gamebox auch Personen die die Hardware
(Mikrocontroller + Display) (noch) nicht haben zu zeigen.

---------------------------AVR Step-by-Step Anleitung --------------------------
1.Anpassen der Datei "avr/Makefile"
1.1 "MCU"
   In der Datei muss der passende MCU eingestellt werden. Die
   Standardeinstellung ist ein ATMGEGA32. Bei einem ATMEGA16 muss die
   Auswahl der Module begrenzt werden. Des weiteren ist nicht ausgeschlossen
   dass für die Module "modul_pxxo", "modul_psnake" und "modul_prev" die ein
   KB RAM des ATMEGA16 ausreichen. (Der ATMEGA32 hat 2KB RAM).
   Ein ATMEGA8 müsste auch möglich sein, jedoch kann aufgrund des knappen
   Programm Speichers nur wahlweise das Modul "modul_prace" oder
   "modul_psnake" verwendet werden. Beim "modul_psnake" können die gleichen
   Probleme wie beim ATMEGA16 auftreten. Des weiteren muss der Quellcode
   bei der Verwendung eines ATMEGA8 noch weitere kleine Änderungen erhalten.
   Auswahlmöglichkeiten für MCU = sind also "atmega32", "atmega16" oder
   "atmega8". Wieweit der Quelltext auch mit anderen AVRs funktioniert wurde
   nicht überprüft.
   Wirklich in der Praxis getestet ist nur der ATMEGA32 mit 8MHZ.
2.Anpassen der Datei "avr/main.h"
2.1 "F_CPU"
   Für "F_CPU" muss der verwendete Takt eingestellt werden. 4MHZ sind das
   Minimum. Es ist nicht auszuschließen dass das Timing oberhalb von 11MHZ
   falsch berechnet wird, dies hätte jedoch höchstens minimale Auswirkungen.
   Standard ist 8MHZ also F_CPU = 8000000
2.2 "osccalreadout"
   Wird der interne Taktgenerator des AVRs verwendet so kann hier das
   Kalibrierungsbyte eingestellt werden. Der passende Wert kann mit dem
   verwendetem Programmer ausgelesen werden. Bei der Verwendung eines Quarzes
   oder einer anderen externen Taktes hat der Wert keine Bedeutung.
2.3 Die Module:
   Hier kann eingestellt werden, welche Programmteile verwendet werden sollen.
   Das "modul_plaby" ist noch nicht vorhanden und kann daher nicht ausgewählt
   werden.
   Eine "1" bedeutet das Modul wird verwendet, bei einer "0" wird auf das Modul
   verzichtet. Alle derzeitigen Module benötigen zusammen rund 24,5KB Flash.
   Bei einem kleinerem AVR muss also auf manche Module verzichtet werden.
   Das "Grundgerüst" aus Menü, Displayansteuerung, Textausagabe u.s.w benötigt
   rund 7KB Flash.
2.3.1 modul_demo
   Ist die Grafik-Demo, welche es auch als Stand-alone Version gibt.
   Hinweis: Die Demo benötigt die Fließkomma Bibliothek. Parameter "-lm" in
   der Datei "Makefile".
   Code Größe: rund 5,5KB
2.3.2 modul_calib_save
   Ermöglicht das Speichern der Joystick Kalibrierung im EEPROM
   Code Größe: knapp 1KB
2.3.3 modul_sram
   Ermittelt die kleinste Stack und größte Heap Adresse. Die Differenz
   wird als minimaler freier Speicher ausgegeben.
   Code Größe: knapp 300Byte
2.3.4 modul_highscore
   Ermöglicht die höchsten erreichten Punkte in den Spielen im EEPROM
   abzuspeichern.
   Code Größe: knapp 1KB
2.3.5 modul_ptetris
   Das Spiel Tetris. Eine Reihe Abbauen: 1 Punkt. Zwei Reihen
   gleichzeitig: 3 Punkte, drei Reihen gleichzeitig 5 Punkte und 4 Reihen
   gleichzeitig bringen 8 Punkte. Links wird das derzeitige Level und die
   abgebauten Reihen angezeigt.
   Code Größe: rund 2KB
2.3.6 modul_prace
   Eine Art Rennspiel. Hindernissen muss ausgewichen werden. Links wird das
   Level und rechts die Lebenspunkte angezeigt. Das Berühren von grünen
   Hindernissen bringt einem Lebenspunkt, rote ziehen einem Lebenspunkt ab.
   Code Größe: gut 0,5KB
2.3.7 modul_pxxo
   Vier Gewinnt wahlweise für ein oder zwei Spieler. Der Computer Gegner
   ist noch verbesserungsbedürftig und sehr langsam. Hier ist ein höherer
   AVR Takt von Vorteil. Der CPU Gegner benötigt auch eine Menge Stack,
   möglicherweise zu viel für 1KB RAM.
   Code Größe: gut 4KB
2.3.8 modul_ppong
   Pong mit zwei Spielvarianten und verschiedenen Schwierigkeitsgrade. Modi 1
   bietet einen perfekten Gegner und es geht darum möglichst lange
   durchzuhalten. Modi 2 bis 4 bieten die Möglichkeit sich gegenseitig
   abzuschießen. Je höher der Modi, desto intelligenter der Gegner.
   Code Größe: gut 2KB
2.3.9 modul_prev
   Reversi für ein oder zwei Spieler. Die Suchtiefe ist auf 4 Halbzüge
   beschränkt. Zu Beginn des Spiels scheint die KI recht gut zu spielen,
   mangels einer gezielten Strategie ist Gewinnen jedoch relativ leicht.
   Die KI benötigt viel RAM und wird daher wahrscheinlich nur auf einem
   ATMEGA32 laufen. Am unteren Rand der Anzeige wird die Anzahl der von der KI
   bewerteten Spielmöglichkeiten binär angezeigt.
   Code Größe: knapp 3KB
2.3.10 modul_psnake
   Snake, die rote Nahrung sammeln bringt Punkte und verlängert die Schlange.
   Je schneller man ist desto mehr Punkte bekommt man! Snake benötigt eine Menge
   Heap, so dass 1KB RAM möglicherweise nicht reichen. In diesem Fall wird eine
   Fehlermeldung angezeigt.
   Code Größe: gut 1KB
2.3.11 modul_xmas
   Ein Weihnachtsstern. Größe und Anzahl der Zacken (2...12) ist über den
   Joystick konfigurierbar. Das Muster wird zunächst als Vektor berechnet und
   dann mit Antialiasing gezeichnet. Wird kein Eingabegerät ausgewähl, startet
   die Anzeige des zuletzt eingestellten Sterns nach 20sec automatisch.
   Code Größe: 2,5KB

3.Anpassen der Datei "avr/graphicint.h"
   Die Pin Belegung des LED Moduls kann hier eingestellt werden.
   Standardmäßig sind alle Anschlüsse auf PORTC gelegt. Beim ATMEGA8 ist das
   jedoch der Port mit dem A/D Wandler und würde somit für den Joystick benötigt
   werden. Ich hoffe die ganzen #defines sind dort selbsterklärend. Generell
   könnte jeder beliebige Anschluss des LED Modules an einen beliebigen Pin des
   AVRs angeschlossen werden. Dies ist auch Port übergreifend möglich!
4.Anpassen der Datei "avr/userinput.h"
   Alle Anschlüsse des Joystick als Eingabegerät müssen an dem Port mit dem A/D
   Wandler liegen. Welcher Pin des Ports mit welchem Anschluss belegt wird ist
   egal, dies kann beliebig eingestellt werden. Als Port muss beim ATMEGA16/32
   PORTA gewählt werden. Beim ATMEGA8 wäre es PORTC. Achtung: Standardmäßig
   liegt an PORTC das LED Modul. Die Ports können hier nicht für beides
   verwendet werden!
5.Erzeugen der .hex Datei
   In der Kommandozeile in das passende Verzeichnis /avr wechseln und "make"
   aufrufen. Läuft alles nach Plan, so wird eine Datei "main.hex" erzeugt, die
   dann mit einem Programmer in den AVR kopiert werden kann.
   Hinweis: Ich will nicht ausschließen dass die "Makefile" unter Windows
   Probleme bereitet. In diesem Fall ist es am einfachsten mit dem Programm
   "mfile" eine neue Datei "Makefile" zu erzeugen. Die zusätzlichen Sourcen
   eintragen und den MCU auswählen. Für das Modul "modul_demo" wird die
   Fließkomma Bibliothek benötigt.

----------------------------- PC Step-by-Step Anleitung ------------------------
Die Anpassung der zu Compilierenden Module erfolgt wie bei der AVR Anleitung
oben in den Punkten 2.3 nur eben in der Datei i386/main.h. Das Modul calib_save
(2.3.2) macht auf dem PC keinen Sinn und sollte daher nicht aktiviert
werden. Das Modul highscore (2.3.4) vergisst derzeit noch die Punkte
sobald das Programm beendet wird.
Das Modul sram (2.3.3) lässt sich zwar fehlerfrei Compilieren, die Ausgabe
führt jedoch zu fehlerhaften Werten da ein PC eben mehr als 64KB RAM hat...
Das restliche Programm wird durch das Modul aber nicht negativ beeinflusst.
Zur Compilierung wird die GLUT Bibliothek benötigt. Es sollten daher Pakete mit
den Namen freeglut3, freeglut3-dev und libglut3 installiert werden wenn die
Compilierung fehlschlägt. Unter Ubuntu lassen sich diese beispielsweise mit
>>sudo apt-get install freeglut3 freeglut3-dev libglut3<<
erledigen.
Falls gcc Version 3.4.x vorhanden ist, sollte wenn möglich diese verwendet
werden (siehe Bugs unten). Dazu muss in der Datei /i386/Makefile in Zeile 74
der Parameter von "CC = gcc" auf "CC = gcc-3.4" geändert werden.
Ein Aufruf von make im Verzeichnis /i386 erzeug das Programm. Starten lässt sich
das compilierte Programm danach mit ./main.elf

------------------------------ Eigene Spiele schreiben -------------------------
Wer eigene Spiele schreiben möchte schaut sich am besten mal den Quellcode der
anderen Spiele an. "graphicfunctions.h" stellt einige einfache Grafikfunktionen
wie das Zeichnen einer Linie oder Box zur Verfügung. Mit "userinput.h" ist es
möglich Benutzeriengaben abzufragen. Timer0 und Timer2 werden bereits verwendet.
Der 16Bit Timer1 steht jedoch zur Verfügung, auch wenn keine Interrupts möglich
sind. In "menu.c" können weitere Menü Einträge hinzugefügt werden.
Falls jemand ein Spiel geschrieben hat oder zumindest eine Idee für eins hat,
würde ich mich über eine Mail freuen.

------------------------------ Bugs --------------------------------------------
AVR:
Ich werde das Gefühl nicht los, dass bei der Joystick Auswertung der Y Achse
irgend etwas faul ist. Manchmal wird eine Bewegung nach oben fälschlicherweise
als eine nach unten interpretiert. Ich weiß nicht ob dies an der Hard- oder
Software liegt. Die X Achse funktioniert einwandfrei und verwendet nahezu den
selben Quellcode.

PC:
Das Timing ist nicht optimal, dies könnte in seltenenen Fällen zu Fehlern in den
Spielen führen. Beobachtet habe ich bisher aber keine.
Mit der gcc 4.0.2 wird an einer Stelle das Menu nicht richtig dargestellt,
außerdem wird fälschlicherweise wegen eine nicht inititialisierten Variable
gewarnt. Falls möglich sollte gcc 3.4.5 verwendet werden, hier treten die oben
genannten Fehler nicht auf.

------------------------------ Changelog ---------------------------------------
Version 1.02 Datum: 2013-12-18
  i386 Timing verbessert
  Weihnachtssternmodus ohne Joystick, startet nach 20sec ohne automatisch

Version 1.01 Datum: 2009-07-24
  Bug im Textrenderer korrigiert

Version: 1.00 (Version 1) Datum: 2006-11-03
  Verwendung von sizeof() korrigiert
  Kommentare aktualisiert
  XXO: Kleinere Bugs behoben
  Programm steht jetzt definitiv unter GPL
  Viele sehr kleine Code Verbesserungen und Korrekturen

Ältere Version: 0.50 (Beta 2) Datum: 2006-10-19
  modul_calib_save: Bug behoben der Beta 1 nicht korrekt laufen ließ
  Reversi: Binäranzeige für die bewerteten KI Züge
  Reversi: Reduzieren der Suchtiefe um einen Halbzug für mehr Geschwindigkeit

Ältere Version: 0.40 (Beta 1) Datum: 2006-10-04 (nie veröffentlicht)
  Erhebliche Verbesserung des XXO Gegners
  Schreiben von Reversi
  Anpassen an die avr-libc 1.4

Ältere Version: 0.30 (Alpha 3) Datum: 2006-05-19
  Bugfix beim Timing der PC Version
  Erweitern der readme.txt

Ältere Version: 0.20 (Alpha 2) Datum: 2006-03-12  (nie veröffentlicht)
  Unterstützung der Compilierung auf dem PC

Ältere Version: 0.10 (Alpha 1) Datum: 2005-08-20
Die Software basiert auf meiner Stand-alone Grafik Demo. Version 1.0

*END OF FILE*