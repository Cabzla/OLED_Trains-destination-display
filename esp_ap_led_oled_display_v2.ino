#include <WiFi.h>          // Bibliothek für WLAN-Funktionen
#include <U8g2lib.h>       // Bibliothek für OLED-Display-Funktionen

// OLED Display Einstellungen
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Zugangspunkt SSID und Passwort
const char* ssid = "ESP32_AP";       // Name des WLANs
const char* password = "12345678";   // Passwort des WLANs

// Webserver auf Port 80
WiFiServer server(80);

// Display-Informationen
char gleis[5] = "11";                        // Gleisnummer
char uhrzeit[6] = "12:00";                   // Uhrzeit
char zugnummer[8] = "ICE 123";               // Zugnummer
char zugname[8] = "";                        // Zugname (optional)
char ziel[17] = "Berlin Hbf";                // Zielort
char zuglauf1[21] = "Hamburg - Hannover";    // Zuglauf Teil 1
char zuglauf2[21] = "- Braunschweig";        // Zuglauf Teil 2
char wagenstand[10] = "111211";              // Wagenstandsanzeiger
char lauftext[85] = "                    +++ Verspaetung ca 10 Min +++";  // Lauftext

const unsigned int lauftextlength = 21;  // Länge des Lauftexts
int offset = 0;       // Startposition für den Lauftext
int subset = 0;       // Zwischenposition für den Lauftext
char ausgabe[lauftextlength + 1];  // Temporäre Zeichenkette für den Lauftext

// Timer für den Lauftext
unsigned long previousMillisLauftext = 0;
const long intervalLauftext = 50; // Intervall in Millisekunden für die Lauftext-Aktualisierung

void setup() {
  Serial.begin(115200);

  // Initialisiert das OLED Display
  u8g2.begin();
  u8g2.setContrast(150);
  u8g2.setFlipMode(1);

  // Startet den Access Point (WLAN)
  WiFi.softAP(ssid, password);
  Serial.println("Access Point gestartet");

  // Gibt die IP-Adresse des Access Points aus
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.softAPIP());

  // Startet den Webserver
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Neuer Client verbunden");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Aktualisiert die Display-Informationen basierend auf der HTTP-Anfrage
    if (request.indexOf("updateDisplay") != -1) {
      updateDisplayInfo(request);
    }

    // Sendet eine Antwort an den Client
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<p>Display Information Updated</p>");
    client.println("</html>");
    client.stop();
    Serial.println("Client Verbindung geschlossen");
  }

  // Aktualisiert die Anzeige
  u8g2.firstPage();
  do {
    draw();
  } while (u8g2.nextPage());

  // Bewegt den Lauftext basierend auf dem Timer
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisLauftext >= intervalLauftext) {
    previousMillisLauftext = currentMillis;

    subset += 1;
    if (subset > 3) {
      offset += 1;
      subset = 0;
    }

    if (offset > strlen(lauftext)) {
      offset = 0;
    }
  }

  // Kein delay() notwendig
}

// Zeichnet die Anzeige
void draw() {
  u8g2.setDrawColor(1);  // Setzt die Zeichenfarbe auf Weiß

  // Zeichnet die Uhrzeit
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 8, uhrzeit);

  // Zeichnet die Zugnummer
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 16, zugnummer);

  // Zeichnet den Wagenstand, wenn vorhanden
  if (strlen(wagenstand) > 0) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 24, "ABCDEFG");

    // Berechnet die Breite basierend auf der Schriftart
    int charWidth = 4; // Jeder Buchstabe ist 4 Pixel breit
    int boxWidth = strlen(wagenstand) * charWidth;
    int boxHeight = 8; // Höhe der Box

    // Zeichnet den weißen Hintergrund für den Wagenstand
    u8g2.drawBox(0, 26, boxWidth, boxHeight);

    // Zeichnet die schwarzen Zahlen auf dem weißen Hintergrund
    u8g2.setDrawColor(0);
    u8g2.drawStr(0, 31, wagenstand); // Y-Koordinate angepasst

    // Setzt die Zeichenfarbe zurück auf Weiß
    u8g2.setDrawColor(1);
  }

  int zuglaufSpalte = 30;  // Startposition für den Zuglauftext

  // Zeichnet den Lauftext, wenn vorhanden
  if (lauftext[20] != ' ') {
    u8g2.setDrawColor(1);
    // Höhe des weißen Balkens reduziert
    u8g2.drawBox(zuglaufSpalte, 0, 78, 7);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_4x6_tf);

    // Clipping-Fenster angepasst
    u8g2.setClipWindow(zuglaufSpalte, 0, zuglaufSpalte + 77, 6);

    int remaining = strlen(lauftext) - offset;
    if (remaining > (lauftextlength - 1)) {
      remaining = lauftextlength - 1;
    }
    memcpy(ausgabe, &lauftext[offset], remaining);
    ausgabe[remaining] = '\0';

    int text_x = zuglaufSpalte - subset;

    // Y-Koordinate des Lauftexts um 1 Pixel nach oben verschoben
    u8g2.drawStr(text_x, 6, ausgabe);

    // Hebt das Clipping-Fenster wieder auf
    u8g2.setMaxClipWindow();

    // Zeichenfarbe und Schriftart zurücksetzen
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_4x6_tf);
  }

  // Zeichnet den Zuglauf Teil 1
  u8g2.drawStr(zuglaufSpalte, 12, zuglauf1);

  // Zeichnet den Zuglauf Teil 2
  u8g2.drawStr(zuglaufSpalte, 19, zuglauf2);

  // Zeichnet das Ziel
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(zuglaufSpalte, 32, ziel);

  // Zeichnet die Gleisnummer
  u8g2.setFont(u8g2_font_6x13B_tf);
  u8g2.drawStr(128 - u8g2.getStrWidth(gleis), 13, gleis);
}

// Aktualisiert die Display-Informationen basierend auf der HTTP-Anfrage
void updateDisplayInfo(String request) {
  if (request.indexOf("uhrzeit=") != -1) {
    urlDecode(getValue(request, "uhrzeit")).toCharArray(uhrzeit, 6);
  }
  if (request.indexOf("zugnummer=") != -1) {
    urlDecode(getValue(request, "zugnummer")).toCharArray(zugnummer, 8);
  }
  if (request.indexOf("zugname=") != -1) {
    urlDecode(getValue(request, "zugname")).toCharArray(zugname, 8);
  }
  if (request.indexOf("ziel=") != -1) {
    urlDecode(getValue(request, "ziel")).toCharArray(ziel, 17);
  }
  if (request.indexOf("zuglauf1=") != -1) {
    urlDecode(getValue(request, "zuglauf1")).toCharArray(zuglauf1, 21);
  }
  if (request.indexOf("zuglauf2=") != -1) {
    urlDecode(getValue(request, "zuglauf2")).toCharArray(zuglauf2, 21);
  }
  if (request.indexOf("wagenstand=") != -1) {
    urlDecode(getValue(request, "wagenstand")).toCharArray(wagenstand, 10);
    Serial.print("Wagenstand aktualisiert: ");
    Serial.println(wagenstand);
  }
  if (request.indexOf("lauftext=") != -1) {
    urlDecode(getValue(request, "lauftext")).toCharArray(lauftext, 85);
  }
  if (request.indexOf("gleis=") != -1) {
    urlDecode(getValue(request, "gleis")).toCharArray(gleis, 5);
  }
}

// Extrahiert den Wert eines Schlüssels aus der HTTP-Anfrage
String getValue(String data, String key) {
  int startIndex = data.indexOf(key + "=") + key.length() + 1;
  int endIndex = data.indexOf("&", startIndex);
  if (endIndex == -1) {
    endIndex = data.indexOf(" ", startIndex);
  }
  if (endIndex == -1) {
    endIndex = data.length();
  }
  return data.substring(startIndex, endIndex);
}

// Dekodiert eine URL-kodierte Zeichenkette
String urlDecode(String str) {
  String decoded = "";
  char temp[] = "00";
  unsigned int len = str.length();
  for (unsigned int i = 0; i < len; i++) {
    if (str[i] == '%') {
      if (i + 2 < len) {
        temp[0] = str[i + 1];
        temp[1] = str[i + 2];
        decoded += (char) strtol(temp, NULL, 16);
        i += 2;
      }
    } else if (str[i] == '+') {
      decoded += ' ';
    } else {
      decoded += str[i];
    }
  }
  return decoded;
}