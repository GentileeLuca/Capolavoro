/*

NOTE:
  QUANDO IN FUNZIONE IL MONITOR SERIALE NON SI POSSONO USARE PIN 0 e 1!


*/
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#include <SPI.h>
#include <RFID.h>
#include <Keypad.h>

// Definizione della mappatura per il keypad
const byte ROWS = 2; // Numero di righe della matrice del keypad
const byte COLS = 2; // Numero di colonne della matrice del keypad

char hexaKeys[ROWS][COLS] = { // Mappatura dei caratteri sulla matrice del keypad
    {'2', '5'},
    {'1', '4'}};

byte rowPins[ROWS] = {4, 5}; // Pin a cui sono collegati i pin di riga del keypad
byte colPins[COLS] = {2, 3}; // Pin a cui sono collegati i pin di colonna del keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); // Inizializzazione del keypad con la mappatura definita

// Dichiarazione delle costanti per i pin
float distance;                                    // Variabile per la distanza rilevata
const int sensorDistance = 10;                     // Distanza di soglia per il rilevamento del veicolo
const int sbarra1Pin = 17;                         // Pin per il servo motore della sbarra 1
const int sbarra2Pin = 16;                         // Pin per il servo motore della sbarra 2
const int pinRossoAnteriore = 0;                   // Pin per il led rosso anteriore
const int pinRossoPosteriore = 1;                  // Pin per il led rosso posteriore
const int HCR1Trig = 6;                            // Pin di trigger per il sensore di distanza HCR1
const int HCR1Echo = 7;                            // Pin di eco per il sensore di distanza HCR1
const int HCR2Trig = 15;                           // Pin di trigger per il sensore di distanza HCR2
const int HCR2Echo = 14;                           // Pin di eco per il sensore di distanza HCR2
const int RFIDSDAPin = 10;                         // Pin per il bus dati I2C del lettore RFID
const int PFIDRSTPin = 9;                          // Pin per il reset del lettore RFID
const int buzzerPin = 8;                           // Pin per il buzzer
const char codiceCarta1[4] = {'1', '5', '4', '2'}; // Codice della carta 1 per il keypad numerico
const char codiceCarta2[4] = {'1', '1', '4', '5'}; // Codice della carta 2 per il keypad numerico

const String carta1 = "d36bbe11"; // Codice della carta 1 per il lettore RFID
const String carta2 = "23c9d90d"; // Codice della carta 2 per il lettore RFID
unsigned char status;
unsigned char str[MAX_LEN];

Servo sbarra1; // Oggetto servo per la sbarra 1
Servo sbarra2; // Oggetto servo per la sbarra 2

LiquidCrystal_I2C lcd(0x27, 16, 2); // Oggetto LCD per il display I2C

RFID carta(RFIDSDAPin, PFIDRSTPin); // Oggetto RFID per il lettore RFID

void setup()
{
    // Impostazioni dei pin come input o output
    pinMode(pinRossoAnteriore, OUTPUT);
    pinMode(pinRossoPosteriore, OUTPUT);
    pinMode(HCR1Trig, OUTPUT);
    pinMode(HCR1Echo, INPUT);
    pinMode(HCR2Trig, OUTPUT);
    pinMode(HCR2Echo, INPUT);
    pinMode(buzzerPin, OUTPUT);
    sbarra1.attach(sbarra1Pin); // Collegamento del servo motore ai rispettivi pin
    sbarra2.attach(sbarra2Pin);
    lcd.init();                // Inizializzazione del display LCD
    lcd.backlight();           // Accensione della retroilluminazione
    randomSeed(analogRead(5)); // Inizializzazione del generatore di numeri casuali
    SPI.begin();               // Inizializzazione della SPI
    carta.init();              // Inizializzazione del lettore RFID
}

// Funzione per il setup dell'HC-SR04
float HCSR04Setup(int trigPin, int echoPin)
{
    digitalWrite(trigPin, LOW);                  // Manda un impulso per calcolare la distanza, Imposta il pin di trigger a LOW
    delayMicroseconds(5);                        // Attende
    digitalWrite(trigPin, HIGH);                 // Imposta il pin di trigger a HIGH
    delayMicroseconds(10);                       // Attende
    digitalWrite(trigPin, LOW);                  // Imposta il pin di trigger a LOW
    long long duration = pulseIn(echoPin, HIGH); // Misura la durata del segnale di echo
    return distance = (duration / 2.0 * 0.0343); // Calcola la distanza in base alla durata del segnale di echo
}

// Funzione per aprire la sbarra
void apriSbarra(Servo sbarra)
{
    for (int pos = 0; pos <= 100; pos += 1) // Apre la sbarra gradualmente
    {
        sbarra.write(pos);
        delay(15);
    }
    return;
}

// Funzione per chiudere la sbarra
void chiudiSbarra(Servo sbarra)
{
    for (int pos = 100; pos >= 0; pos -= 1) // Chiude la sbarra gradualmente
    {
        sbarra.write(pos);
        delay(15);
    }
    return;
}

// Funzione per leggere la carta RFID
String rfid()
{
    String cardCode = "";   // Stringa per il codice della carta RFID
    bool cardReaded = true; // Flag per indicare se la carta è stata letta
    while (cardReaded)
    {
        if (carta.findCard(PICC_REQIDL, str) == MI_OK) // Se una carta è rilevata
        {
            if (carta.anticoll(str) == MI_OK) // Se l'anticollisione ha successo
            {

                for (int i = 0; i < 4; i++) // Legge il codice della carta
                {
                    cardCode += String(0x0F & (str[i] >> 4), HEX);
                    cardCode += String(0x0F & str[i], HEX);
                }
                cardReaded = false; // Imposta il flag a false per uscire dal loop
            }
            carta.selectTag(str); // Seleziona la carta per ulteriori operazioni
        }
    }
    carta.halt();    // Mette la carta in stato di riposo
    return cardCode; // Restituisce il codice della carta
}

// Funzione per scrivere sul display LCD
void lcdDisplay()
{
    lcd.setCursor(0, 0);
    lcd.print("BENVENUTO       ");
    lcd.setCursor(0, 1);
    lcd.print("INSERISCI CARTA");
}

// Funzione per scrivere sul display LCD
void lcdWriteCardRead(int prezzo)
{
    lcd.setCursor(0, 0);
    lcd.print("IMPORTO: ");
    lcd.print(prezzo);
    lcd.print(" euro");
    lcd.setCursor(0, 1);
    lcd.print("INERISCI PIN   ");
}

// Funzione per cancellare il contenuto del display LCD
void clearLcd()
{
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
}

// Funzione per inizializzare il display LCD
void startLcd()
{
    lcd.setCursor(0, 0);
    lcd.print("CASELLO NUMERO 1");
    lcd.setCursor(0, 1);
    lcd.print("SOLO CARTE      ");
}

// Funzione per indicare un pin errato
void wrongPin()
{
    lcd.setCursor(0, 0);
    lcd.print("PIN ERRATO      ");
    lcd.setCursor(0, 1);
    lcd.print("RITENTARE       ");
}

// Funzione per scrivere un messaggio di arrivederci sul display LCD
void lcdWriteGoodbyeMessage()
{
    lcd.setCursor(0, 0);
    lcd.print("GRAZIE E        ");
    lcd.setCursor(0, 1);
    lcd.print("ARRIVEDERCI     ");
}

// Funzione per scrivere un messaggio sulla mancanza della carta sul display LCD
void lcdWriteNoCardMsg()
{
    lcd.setCursor(0, 0);
    lcd.print("CARTA NON VALIDA");
    lcd.setCursor(0, 1);
    lcd.print("INERISCI N TARGA");
}

// Funzione per scrivere un messaggio di pagamento senza carta sul display LCD
void lcdPaymentNoCard(int prezzo)
{
    lcd.setCursor(0, 0);
    lcd.print("NUMERO TARGA    ");
    lcd.setCursor(0, 1);
    lcd.print("RICEVUTO        ");
    delay(500);
    lcd.setCursor(0, 0);
    lcd.print("LE VERRANNO     ");
    lcd.setCursor(0, 1);
    lcd.print("ADDEBITATI      ");
    lcd.print(prezzo);
    lcd.print(" e");
}

// Funzione per indicare una transazione eseguita sul display LCD
void lcdVer()
{
    lcd.print("TRANSAZIONE     ");
    lcd.setCursor(0, 1);
    lcd.print("ESEGUITA        ");
}

// Funzione per indicare un pin errato
void wrong()
{
    clearLcd();
    wrongPin();
    tone(buzzerPin, 523);
    delay(500);
    noTone(buzzerPin);
}

// Funzione per indicare che tutto è ok
void ok()
{
    clearLcd();
    lcd.setCursor(0, 0);
    lcd.print("       ok       ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
}

// Funzione per far suonare il buzzer a 400 Hz
void buzz400()
{
    tone(buzzerPin, 800);
    delay(100);
    noTone(buzzerPin);
}

// Funzione per far suonare il buzzer a 800 Hz
void buzz800()
{
    tone(buzzerPin, 400);
    delay(100);
    noTone(buzzerPin);
}

void loop()
{
    startLcd();                                       // Scrive sul display LCD
    sbarra1.write(0);                                 // Imposta il servo motore nella posizione di chiusura
    sbarra2.write(0);                                 // Imposta il servo motore nella posizione di chiusura
    digitalWrite(pinRossoAnteriore, LOW);             // Spegne il led rosso anteriore
    digitalWrite(pinRossoPosteriore, HIGH);           // Accende il led rosso posteriore
    float distance = HCSR04Setup(HCR1Trig, HCR1Echo); // Misura la distanza dal sensore HCR1
    delay(250);
    if (distance <= sensorDistance) // Se la distanza è minore o uguale alla distanza di soglia
    {
        digitalWrite(pinRossoAnteriore, HIGH);                                           // Accende il led rosso anteriore
        tone(buzzerPin, 392);                                                            // Suona il buzzer
        apriSbarra(sbarra1);                                                             // Apre la sbarra
        noTone(buzzerPin);                                                               // Spegne il buzzer
        float externalDistance = HCSR04Setup(HCR1Trig, HCR1Echo);                        // Misura la distanza esterna
        float internalDistance = HCSR04Setup(HCR2Trig, HCR2Echo);                        // Misura la distanza interna
        while (internalDistance >= sensorDistance || externalDistance <= sensorDistance) // Finché le distanze non sono corrette
        {
            internalDistance = HCSR04Setup(HCR2Trig, HCR2Echo); // Ricalcola la distanza interna
            externalDistance = HCSR04Setup(HCR1Trig, HCR1Echo); // Ricalcola la distanza esterna
        }
        delay(1500);
        digitalWrite(pinRossoAnteriore, HIGH); // Accende il led rosso anteriore
        tone(buzzerPin, 698);                  // Suona il buzzer
        chiudiSbarra(sbarra1);                 // Chiude la sbarra
        noTone(buzzerPin);                     // Spegne il buzzer
        lcdDisplay();                          // Scrive sul display LCD
        delay(1500);
        buzz400();
        buzz400();
        String cardCode = rfid(); // Legge il codice della carta RFID
        buzz800();
        int witchCard = 0;

        if (cardCode == carta1) // Verifica il tipo di carta
        {
            witchCard = 1;
        }
        else if (cardCode == carta2)
        {
            witchCard = 2;
        }
        else
        {
            witchCard = 3;
        }
        ok();
        delay(500);
        int prezzo = random(1, 99); // Genera un prezzo casuale
        if (witchCard == 3)
        {
            lcdWriteNoCardMsg(); // Scrive sul display LCD il messaggio di carta non valida
        }
        else
        {
            lcdWriteCardRead(prezzo); // Scrive sul display LCD il messaggio di lettura della carta
        }
        bool v = false;
        do
        {
            int conta = 0;
            char inputCode[4];
            while (conta < 4) // Legge i tasti del keypad
            {
                char key = customKeypad.getKey();
                if (key)
                {
                    inputCode[conta] = key;
                    conta += 1;
                    tone(buzzerPin, 400);
                    delay(100);
                    noTone(buzzerPin);
                    delay(100);
                }
            }
            for (int i = 0; i < 4; i++) // Verifica il pin inserito
            {
                if (witchCard == 1)
                {
                    if (inputCode[i] != codiceCarta1[i])
                    {
                        wrong();
                        v = true;
                        break;
                    }
                }
                else if (witchCard == 2)
                {
                    if (inputCode[i] != codiceCarta2[i])
                    {
                        wrong();
                        v = true;
                        break;
                    }
                }
                v = false;
            }
        } while (v);
        if (witchCard == 3)
        {
            lcdPaymentNoCard(prezzo); // Scrive sul display LCD il messaggio di pagamento senza carta
        }
        else
        {
            lcdVer(); // Scrive sul display LCD il messaggio di transazione eseguita
        }
        digitalWrite(pinRossoPosteriore, LOW); // Spegne il led rosso posteriore
        tone(buzzerPin, 392);                  // Suona il buzzer
        lcdWriteGoodbyeMessage();              // Scrive sul display LCD il messaggio di arrivederci
        delay(100);
        apriSbarra(sbarra2);                                       // Apre la sbarra 2
        noTone(buzzerPin);                                         // Spegne il buzzer
        float internalDistance2 = HCSR04Setup(HCR2Trig, HCR2Echo); // Misura la distanza interna
        bool exitBool = true;
        while (internalDistance2 <= sensorDistance) // Finché la distanza interna è minore o uguale alla distanza di soglia
        {
            internalDistance2 = HCSR04Setup(HCR2Trig, HCR2Echo); // Ricalcola la distanza interna
        }
        delay(1500);
        tone(buzzerPin, 698);                   // Suona il buzzer
        chiudiSbarra(sbarra2);                  // Chiude la sbarra 2
        noTone(buzzerPin);                      // Spegne il buzzer
        digitalWrite(pinRossoPosteriore, HIGH); // Accende il led rosso posteriore
        digitalWrite(pinRossoAnteriore, LOW);   // Spegne il led rosso anteriore
        startLcd();                             // Scrive sul display LCD
    }
    delay(500);
}