Titel:    Gamebox mit SAMSUNG SLM1608 Display
Autor:    (c) 2004-2006 by Malte Marwedel
Datum:    2006-11-03
Version:  1.00 (Final 1)
Zweck:    Spiele auf einem Grafikdisplay
Software: GCC-AVR / GCC
Hardware: LED Panel, ATMEGA32 oder �hnlichen mit 8MHZ.
          Einzelne Module m�ssten auch auf einem ATMEGA16 laufen.
          "modul_pxxo" und "modul_psnake" k�nnten jedoch mangels RAM
          m�glicherweise gar nicht oder nur fehlerhaft laufen.
          ODER:
          PC mit 500MHZ und 800x600 @ 256 Farben 3D beschleunigte Grafik
Bemerkung:Der Quellcode steht unter der GPL. Der Lizenztext kann in der Datei
          gpl-license.txt gefunden werden.
          Ausgenommen hiervon sind die Dateien "avr/Makefile" und
          "i386/Makefile". Sie entstammt dem Programm "mfile" und f�r die Datei
          gelten entsprechende andere Bedingungen f�r die Verwendung.
Wer Fragen oder Anregungen zu dem Programm hat, kann an
           m.marwedel <AT> onlinehome.de mailen.
          Mehr �ber Elektronik und AVRs gibt es auf meiner Homepage:
           http://www.marwedels.de/malte/
Code Gr��e:ATMEGA32: 29710 Byte (getestet, alle Module)
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
Die Software wurde urspr�nglich nur zum Ausf�hren auf einem Mikrocontroller
geschrieben. Die M�glichkeit die Software auch auf dem PC zu testen kam erst
sp�ter hinzu. Dabei habe ich versucht den Code m�glichst wenig zu �ndern.
Im prim�rem Verzeichnis befindet sich der vom AVR und PC gemeinsam benutzte
Code. Das Verzeichnis /avr enth�lt den AVR spezifischen und /i386 den PC
spezifischen Code. Der Code ist also optimiert f�r einen Mikrocontroller, sprich
eine Endlosschleife erledigt die meisten Berechnungen und Zeitsteuerungen werden
sowohl �ber Delay Schleifen, als auch �ber einen 16Bit Timer geregelt. Der
Interrupt eines 8Bit Timers sorgt f�r die Grafikausgabe und ein anderer
Interrupt eines zweiten 8Bit Timers sorgt f�r die Joystick Eingabe.
Auf dem PC ist nun die Grafikausgabe und Tasteneingabe grundlegend anders, so
dass hier Anstelle der beiden 8Bit Timer Funktionen der GLUT Libray diese
Aufgabe �bernehmen. Die Hauproutine (Endlosschleife) wird als extra Thread
gestartet und verbraucht somit soviel CPU Rechenzeit wie m�glich. Dies bedeutet,
solange das Programm auf dem PC l�uft liegt die CPU Auslastung kontinuierlich
bei 100%. Dank des Betriebssystems bekommen aber nat�rlich auch andere Prozesse
Rechenzeit ab und der Rechner h�ngt nicht :-)
Die Delayschleife musste neu geschrieben werden und erledigt ihre Aufgabe durch
Auslesen der Systemzeit und Vergleich mit der gew�nschten Delay Zeit.
Der ebenfalls zum Timing herangezogene 16Bit Timer wird simuliert indem Mithilfe
der GLUT Libray periodisch eine Funktion ausgef�hrt wird welche die 16Bit Timer
Variable entsprechend erh�ht. Leider ist das periodische Ausf�hren dieser
Funktion nur mit einer relativ niedrigen Frequenz m�glich, so dass der Timer
immer um mehrer Z�hlschritte auf einmal erh�ht werden muss. Auch ist die Dauer
zwischen dem Ausf�hren der Funktion nicht immer gleich gro�, so dass die
Z�hlschritte mal gr��er und mal kleiner ausfallen. Dies liegt vorallem daran,
dass ein CPU Kern nur einen Thread zur Zeit bearbeiten kann und der Wechsel
zwischen den verschiedenen Threads seltener erfolgt als die Funktion zum Erh�hen
des 16Bit Z�hlers aufgerufen werden m�sste.
Das Resultat davon �u�ert sich darin dass die Software etwas stockend und tr�ge
wirkt. Auf dem AVR sind die Bewegungen deutlich gleichm��iger und das Spielen
der Spiele macht definitiv mehr Spa�!
Das Laufen auf dem PC dient daher eher
1. als Experiment in wie weit Code von der einen Plattform auch auf der andren
l�uft
2. Als Plattform um Fehler schneller zu finden - einfachere Fehlerausgabe, kein
umst�ndlicher Upload in den Mikrocontroller
und 3. um die M�glichkeiten der Gamebox auch Personen die die Hardware
(Mikrocontroller + Display) (noch) nicht haben zu zeigen.

---------------------------AVR Step-by-Step Anleitung --------------------------
1.Anpassen der Datei "avr/Makefile"
1.1 "MCU"
   In der Datei muss der passende MCU eingestellt werden. Die
   Standardeinstellung ist ein ATMGEGA32. Bei einem ATMEGA16 muss die
   Auswahl der Module begrenzt werden. Des weiteren ist nicht ausgeschlossen
   dass f�r die Module "modul_pxxo", "modul_psnake" und "modul_prev" die ein
   KB RAM des ATMEGA16 ausreichen. (Der ATMEGA32 hat 2KB RAM).
   Ein ATMEGA8 m�sste auch m�glich sein, jedoch kann aufgrund des knappen
   Programm Speichers nur wahlweise das Modul "modul_prace" oder
   "modul_psnake" verwendet werden. Beim "modul_psnake" k�nnen die gleichen
   Probleme wie beim ATMEGA16 auftreten. Des weiteren muss der Quellcode
   bei der Verwendung eines ATMEGA8 noch weitere kleine �nderungen erhalten.
   Auswahlm�glichkeiten f�r MCU = sind also "atmega32", "atmega16" oder
   "atmega8". Wieweit der Quelltext auch mit anderen AVRs funktioniert wurde
   nicht �berpr�ft.
   Wirklich in der Praxis getestet ist nur der ATMEGA32 mit 8MHZ.
2.Anpassen der Datei "avr/main.h"
2.1 "F_CPU"
   F�r "F_CPU" muss der verwendete Takt eingestellt werden. 4MHZ sind das
   Minimum. Es ist nicht auszuschlie�en dass das Timing oberhalb von 11MHZ
   falsch berechnet wird, dies h�tte jedoch h�chstens minimale Auswirkungen.
   Standard ist 8MHZ also F_CPU = 8000000
2.2 "osccalreadout"
   Wird der interne Taktgenerator des AVRs verwendet so kann hier das
   Kalibrierungsbyte eingestellt werden. Der passende Wert kann mit dem
   verwendetem Programmer ausgelesen werden. Bei der Verwendung eines Quarzes
   oder einer anderen externen Taktes hat der Wert keine Bedeutung.
2.3 Die Module:
   Hier kann eingestellt werden, welche Programmteile verwendet werden sollen.
   Das "modul_plaby" ist noch nicht vorhanden und kann daher nicht ausgew�hlt
   werden.
   Eine "1" bedeutet das Modul wird verwendet, bei einer "0" wird auf das Modul
   verzichtet. Alle derzeitigen Module ben�tigen zusammen rund 24,5KB Flash.
   Bei einem kleinerem AVR muss also auf manche Module verzichtet werden.
   Das "Grundger�st" aus Men�, Displayansteuerung, Textausagabe u.s.w ben�tigt
   rund 7KB Flash.
2.3.1 modul_demo
   Ist die Grafik-Demo, welche es auch als Stand-alone Version gibt.
   Hinweis: Die Demo ben�tigt die Flie�komma Bibliothek. Parameter "-lm" in
   der Datei "Makefile".
   Code Gr��e: rund 5,5KB
2.3.2 modul_calib_save
   Erm�glicht das Speichern der Joystick Kalibrierung im EEPROM
   Code Gr��e: knapp 1KB
2.3.3 modul_sram
   Ermittelt die kleinste Stack und gr��te Heap Adresse. Die Differenz
   wird als minimaler freier Speicher ausgegeben.
   Code Gr��e: knapp 300Byte
2.3.4 modul_highscore
   Erm�glicht die h�chsten erreichten Punkte in den Spielen im EEPROM
   abzuspeichern.
   Code Gr��e: knapp 1KB
2.3.5 modul_ptetris
   Das Spiel Tetris. Eine Reihe Abbauen: 1 Punkt. Zwei Reihen
   gleichzeitig: 3 Punkte, drei Reihen gleichzeitig 5 Punkte und 4 Reihen
   gleichzeitig bringen 8 Punkte. Links wird das derzeitige Level und die
   abgebauten Reihen angezeigt.
   Code Gr��e: rund 2KB
2.3.6 modul_prace
   Eine Art Rennspiel. Hindernissen muss ausgewichen werden. Links wird das
   Level und rechts die Lebenspunkte angezeigt. Das Ber�hren von gr�nen
   Hindernissen bringt einem Lebenspunkt, rote ziehen einem Lebenspunkt ab.
   Code Gr��e: gut 0,5KB
2.3.7 modul_pxxo
   Vier Gewinnt wahlweise f�r ein oder zwei Spieler. Der Computer Gegner
   ist noch verbesserungsbed�rftig und sehr langsam. Hier ist ein h�herer
   AVR Takt von Vorteil. Der CPU Gegner ben�tigt auch eine Menge Stack,
   m�glicherweise zu viel f�r 1KB RAM.
   Code Gr��e: gut 4KB
2.3.8 modul_ppong
   Pong mit zwei Spielvarianten und verschiedenen Schwierigkeitsgrade. Modi 1
   bietet einen perfekten Gegner und es geht darum m�glichst lange
   durchzuhalten. Modi 2 bis 4 bieten die M�glichkeit sich gegenseitig
   abzuschie�en. Je h�her der Modi, desto intelligenter der Gegner.
   Code Gr��e: gut 2KB
2.3.9 modul_prev
   Reversi f�r ein oder zwei Spieler. Die Suchtiefe ist auf 4 Halbz�ge
   beschr�nkt. Zu Beginn des Spiels scheint die KI recht gut zu spielen,
   mangels einer gezielten Strategie ist Gewinnen jedoch relativ leicht.
   Die KI ben�tigt viel RAM und wird daher wahrscheinlich nur auf einem
   ATMEGA32 laufen. Am unteren Rand der Anzeige wird die Anzahl der von der KI
   bewerteten Spielm�glichkeiten bin�r angezeigt.
   Code Gr��e: knapp 3KB
2.3.10 modul_psnake
   Snake, die rote Nahrung sammeln bringt Punkte und verl�ngert die Schlange.
   Je schneller man ist desto mehr Punkte bekommt man! Snake ben�tigt eine Menge
   Heap, so dass 1KB RAM m�glicherweise nicht reichen. In diesem Fall wird eine
   Fehlermeldung angezeigt.
   Code Gr��e: gut 1KB
2.3.11 modul_xmas
   Ein Weihnachtsstern. Gr��e und Anzahl der Zacken (2...12) ist �ber den
   Joystick konfigurierbar. Das Muster wird zun�chst als Vektor berechnet und
   dann mit Antialiasing gezeichnet. Wird kein Eingabeger�t ausgew�hl, startet
   die Anzeige des zuletzt eingestellten Sterns nach 20sec automatisch.
   Code Gr��e: 2,5KB

3.Anpassen der Datei "avr/graphicint.h"
   Die Pin Belegung des LED Moduls kann hier eingestellt werden.
   Standardm��ig sind alle Anschl�sse auf PORTC gelegt. Beim ATMEGA8 ist das
   jedoch der Port mit dem A/D Wandler und w�rde somit f�r den Joystick ben�tigt
   werden. Ich hoffe die ganzen #defines sind dort selbsterkl�rend. Generell
   k�nnte jeder beliebige Anschluss des LED Modules an einen beliebigen Pin des
   AVRs angeschlossen werden. Dies ist auch Port �bergreifend m�glich!
4.Anpassen der Datei "avr/userinput.h"
   Alle Anschl�sse des Joystick als Eingabeger�t m�ssen an dem Port mit dem A/D
   Wandler liegen. Welcher Pin des Ports mit welchem Anschluss belegt wird ist
   egal, dies kann beliebig eingestellt werden. Als Port muss beim ATMEGA16/32
   PORTA gew�hlt werden. Beim ATMEGA8 w�re es PORTC. Achtung: Standardm��ig
   liegt an PORTC das LED Modul. Die Ports k�nnen hier nicht f�r beides
   verwendet werden!
5.Erzeugen der .hex Datei
   In der Kommandozeile in das passende Verzeichnis /avr wechseln und "make"
   aufrufen. L�uft alles nach Plan, so wird eine Datei "main.hex" erzeugt, die
   dann mit einem Programmer in den AVR kopiert werden kann.
   Hinweis: Ich will nicht ausschlie�en dass die "Makefile" unter Windows
   Probleme bereitet. In diesem Fall ist es am einfachsten mit dem Programm
   "mfile" eine neue Datei "Makefile" zu erzeugen. Die zus�tzlichen Sourcen
   eintragen und den MCU ausw�hlen. F�r das Modul "modul_demo" wird die
   Flie�komma Bibliothek ben�tigt.

----------------------------- PC Step-by-Step Anleitung ------------------------
Die Anpassung der zu Compilierenden Module erfolgt wie bei der AVR Anleitung
oben in den Punkten 2.3 nur eben in der Datei i386/main.h. Das Modul calib_save
(2.3.2) macht auf dem PC keinen Sinn und sollte daher nicht aktiviert
werden. Das Modul highscore (2.3.4) vergisst derzeit noch die Punkte
sobald das Programm beendet wird.
Das Modul sram (2.3.3) l�sst sich zwar fehlerfrei Compilieren, die Ausgabe
f�hrt jedoch zu fehlerhaften Werten da ein PC eben mehr als 64KB RAM hat...
Das restliche Programm wird durch das Modul aber nicht negativ beeinflusst.
Zur Compilierung wird die GLUT Bibliothek ben�tigt. Es sollten daher Pakete mit
den Namen freeglut3, freeglut3-dev und libglut3 installiert werden wenn die
Compilierung fehlschl�gt. Unter Ubuntu lassen sich diese beispielsweise mit
>>sudo apt-get install freeglut3 freeglut3-dev libglut3<<
erledigen.
Falls gcc Version 3.4.x vorhanden ist, sollte wenn m�glich diese verwendet
werden (siehe Bugs unten). Dazu muss in der Datei /i386/Makefile in Zeile 74
der Parameter von "CC = gcc" auf "CC = gcc-3.4" ge�ndert werden.
Ein Aufruf von make im Verzeichnis /i386 erzeug das Programm. Starten l�sst sich
das compilierte Programm danach mit ./main.elf

------------------------------ Eigene Spiele schreiben -------------------------
Wer eigene Spiele schreiben m�chte schaut sich am besten mal den Quellcode der
anderen Spiele an. "graphicfunctions.h" stellt einige einfache Grafikfunktionen
wie das Zeichnen einer Linie oder Box zur Verf�gung. Mit "userinput.h" ist es
m�glich Benutzeriengaben abzufragen. Timer0 und Timer2 werden bereits verwendet.
Der 16Bit Timer1 steht jedoch zur Verf�gung, auch wenn keine Interrupts m�glich
sind. In "menu.c" k�nnen weitere Men� Eintr�ge hinzugef�gt werden.
Falls jemand ein Spiel geschrieben hat oder zumindest eine Idee f�r eins hat,
w�rde ich mich �ber eine Mail freuen.

------------------------------ Bugs --------------------------------------------
AVR:
Ich werde das Gef�hl nicht los, dass bei der Joystick Auswertung der Y Achse
irgend etwas faul ist. Manchmal wird eine Bewegung nach oben f�lschlicherweise
als eine nach unten interpretiert. Ich wei� nicht ob dies an der Hard- oder
Software liegt. Die X Achse funktioniert einwandfrei und verwendet nahezu den
selben Quellcode.

PC:
Das Timing ist nicht optimal, dies k�nnte in seltenenen F�llen zu Fehlern in den
Spielen f�hren. Beobachtet habe ich bisher aber keine.
Mit der gcc 4.0.2 wird an einer Stelle das Menu nicht richtig dargestellt,
au�erdem wird f�lschlicherweise wegen eine nicht inititialisierten Variable
gewarnt. Falls m�glich sollte gcc 3.4.5 verwendet werden, hier treten die oben
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

�ltere Version: 0.50 (Beta 2) Datum: 2006-10-19
  modul_calib_save: Bug behoben der Beta 1 nicht korrekt laufen lie�
  Reversi: Bin�ranzeige f�r die bewerteten KI Z�ge
  Reversi: Reduzieren der Suchtiefe um einen Halbzug f�r mehr Geschwindigkeit

�ltere Version: 0.40 (Beta 1) Datum: 2006-10-04 (nie ver�ffentlicht)
  Erhebliche Verbesserung des XXO Gegners
  Schreiben von Reversi
  Anpassen an die avr-libc 1.4

�ltere Version: 0.30 (Alpha 3) Datum: 2006-05-19
  Bugfix beim Timing der PC Version
  Erweitern der readme.txt

�ltere Version: 0.20 (Alpha 2) Datum: 2006-03-12  (nie ver�ffentlicht)
  Unterst�tzung der Compilierung auf dem PC

�ltere Version: 0.10 (Alpha 1) Datum: 2005-08-20
Die Software basiert auf meiner Stand-alone Grafik Demo. Version 1.0

*END OF FILE*