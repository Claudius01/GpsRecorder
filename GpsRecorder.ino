// $Id: GpsRecorder.ino,v 1.17 2025/02/04 15:03:36 administrateur Exp $

/* Projet: GpsRecorder

   2025/01/10
      - Implementation cadencement tick a 50uS pour distribuer les traitements
        "proches" du temps reel (Reception UART, I2C, etc.)

   2025/01/28
      - Merge with 'SDCard' code...

   2025/02/04
      - Improve presentation...
 */

#include "Misc.h"
#include "Timers.h"
#include "DateTime.h"
#include "GestionLCD.h"
#include "Nmea.h"
#include "SDCard.h"
#include "OneButton.h"

/*                            1   .     2       . 3         4 .       5     .
                     12345678901234567890123456789012345678901234567890123456
*/
#define PROMPT      "..GpsRecorder...2025/02/04....V1.0"    // 3 lignes de 14 caracteres

#define USE_INCOMING_CMD    1

// Pins de l'ESP32-S3-GEEK
#define PIN_LED         6       // GPIO6
#define PIN_INPUT       0       // GPIO0 (Bouton)

// Definitions pour le Timer Hard
#define TIMER_FREQUENCY       1000000    // 1 MHz
//#define TIMER_RATIO           1000       // Ratio for 1 mS
//#define TIMER_RATIO           10000       // Ratio for 100 uS
#define TIMER_RATIO           20000       // Ratio for 50 uS

#define COUNTER_FOR_1_MS      20          // Comptabilisation des 1 mS
#define COUNTER_FOR_1_25_MS   25          // Comptabilisation des 1.25 mS
#define COUNTER_FOR_2_5_MS    50          // Comptabilisation des 2.5 mS
#define COUNTER_FOR_10_MS     200         // Comptabilisation des 10 mS
#define COUNTER_FOR_500_MS    10000       // Comptabilisation des 1/2 secondes
#define COUNTER_FOR_1_SEC     20000       // Comptabilisation des secondes

hw_timer_t    *g__timer = NULL;
portMUX_TYPE  g__timerMux = portMUX_INITIALIZER_UNLOCKED;

byte          g__state_leds = STATE_NO_LEDS;

bool          g__flg_inh_sdcard_ope = false;

Timers        *g__timers = NULL;
Nmea          *g__nmea = NULL;
SDCard        *g__sdcard = NULL;
OneButton     *g__button = NULL;
GestionLCD    *g__gestion_lcd = NULL;
DateTime      *g__date_time = NULL;

#if USE_INCOMING_CMD
size_t        g__count = 0;
char          g__incoming_buff[132 + 1];   // +1 for '\0' terminal
bool          g__flg_wait_command = true;
#endif

uint16_t g__chenillard = 0x8080;

/* Compteur de comptabilisation des passages dans 'onTimer'
  => Remis a zero si 'g__flg_expired_500ms' affirme
*/
volatile uint16_t   g__pass_on_timer = 0;

volatile bool       g__flg_expired_1ms = false;
volatile bool       g__flg_expired_1_25ms = false;
//volatile bool       g__flg_expired_2_5ms = false;
volatile bool       g__flg_expired_10ms = false;
volatile bool       g__flg_expired_500ms = false;

/* Methode de cadencement toutes les 50 uS
   => Cf. 'https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html'
*/
void ARDUINO_ISR_ATTR onTimer()
{
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&g__timerMux);

  g__pass_on_timer++;

  if ((g__pass_on_timer % COUNTER_FOR_1_MS) == 0) {
    g__flg_expired_1ms = true;
  }

  if ((g__pass_on_timer % COUNTER_FOR_1_25_MS) == 0) {
    g__flg_expired_1_25ms = true;
  }

  if ((g__pass_on_timer % COUNTER_FOR_10_MS) == 0) {
    g__flg_expired_10ms = true;
  }

  if ((g__pass_on_timer % COUNTER_FOR_500_MS) == 0) {
    g__flg_expired_500ms = true;
  }

  portEXIT_CRITICAL_ISR(&g__timerMux);
}

void execForever()
{
  /* Si 'PIN_LED' a 0V
     => Mise sur voie de garage dans le cas d'un plantage
        recurrent empechant un nouveau telechargement
     => Clignotement a 10 Hz de la Led
  */
  pinMode(PIN_LED, INPUT_PULLUP);

  if (digitalRead(PIN_LED) == LOW) {
    Serial.print("-> Mise sur voie de garage...\n");

    pinMode(PIN_LED, OUTPUT);

    while (true) {
      digitalWrite(PIN_LED, LOW);
      delay(100);
      digitalWrite(PIN_LED, HIGH);
      delay(100);
    }
  }
}

bool go_to_sleep()
{
  esp_sleep_enable_timer_wakeup(1000);    // Duration in uS
  esp_deep_sleep_start();
}

/* Presentation differents clignotement a partir d'un
   pattern sur 16 bits decales par rotation a gauche
   et maj a l'expiration du timer 'TIMER_CHENILLARD'
*/ 
void callback_exec_chenillard()
{
  bool l__flg_d15 = (g__chenillard & 0x8000) ? true : false;

  g__chenillard <<= 1;
  g__chenillard |= (l__flg_d15 == true) ? 0x0001: 0x0000;   // Report eventuel du bit 15 avant decalage

  if (g__chenillard & 0x0001) {
    g__state_leds |= STATE_LED_GREEN;
    g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_GREEN_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, GREEN);
  }
  else {
    g__state_leds &= ~STATE_LED_GREEN;
    g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_GREEN_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, GREEN);
  }

  g__timers->start(TIMER_CHENILLARD, DURATION_TIMER_CHENILLARD, &callback_exec_chenillard);
}

// This function will be called when the button started long pressed.
void callback_long_press_start(void *oneButton)
{
  Serial.print("callback_long_press_start(): Entering...\n");
  Serial.print("-> Force the restart\n");

  delay(1000);
  go_to_sleep();
}

void callback_click(void *oneButton)
{
  //Serial.print("callback_click(): Entering...\n");

  char l__buffer[32];
  memset(l__buffer, '\0', sizeof(l__buffer));
  g__sdcard->formatSize(g__sdcard->getGpsFrameSize(), l__buffer);

  // Ecriture sur la SDCard du resultat avant l'action
  if (g__sdcard->getInhAppendGpsFrame() == false) {
    g__sdcard->appendGpsFrame("#Stop recording\n");

    g__gestion_lcd->Paint_DrawString_EN(6, 40, l__buffer, &Font16, BLACK, YELLOW);
  }

  // Bascule "Autorisation/Inhibition" concatenation infos dans la SDCard
  g__sdcard->setInhAppendGpsFrame(g__sdcard->getInhAppendGpsFrame() ? false : true);

  // Ecriture sur la SDCard du resultat apres l'action
  if (g__sdcard->getInhAppendGpsFrame() == false) {
    g__sdcard->appendGpsFrame("#Start recording\n");

    g__gestion_lcd->Paint_DrawString_EN(6, 40, l__buffer, &Font16, BLACK, WHITE);
  }
}

void callback_double_click(void *oneButton)
{
  Serial.print("callback_double_click(): Entering...\n");

  g__sdcard->init(3);                            // 10 tentatives
  g__sdcard->callback_sdcard_retry_init_more();
}

void callback_sdcard_retry_init()
{
  // Suite du traitement dans la classe 'SDCard'
  g__sdcard->callback_sdcard_retry_init_more();
}

void callback_end_sdcard_acces() {
  g__state_leds &= ~STATE_LED_YELLOW;
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, YELLOW);
} 
  
void callback_end_sdcard_error() {
  g__state_leds &= ~STATE_LED_RED;
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_RED_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, RED);
}
  
void callback_sdcard_init_error()
{
  g__state_leds |= STATE_LED_RED;

  g__timers->start(TIMER_SDCARD_INIT_ERROR, DURATION_TIMER_SDCARD_INIT_ERROR, &callback_sdcard_init_error);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_RED_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, RED);
}

/* Test the LEDs on before processing
 * - Allumage des 3 Leds
 * - Attente de 1 Sec.
 * - Extinction de la Led Red après 500 mS
 * - Extinction de la Led Yellow après 1 Sec.
 * - Extinction de la Led Green après 1.5 Sec
*/
void testLeds()
{
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);

  g__gestion_lcd->Paint_DrawString_EN(28, 90, "GPS", &Font16, BLACK, WHITE);
  g__gestion_lcd->Paint_DrawString_EN(108, 90, "SDCard", &Font16, BLACK, WHITE);

  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_GREEN_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, GREEN);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, YELLOW);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_RED_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, RED);

  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, YELLOW);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_RED_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, RED);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_BLUE_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, BLUE);

  delay(1000);

  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_BLUE_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, BLUE);
  delay(500);

  digitalWrite(LED_RED, HIGH);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_RED_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, RED);
  delay(500);

  digitalWrite(LED_YELLOW, HIGH);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, YELLOW);
  delay(500);

  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_RED_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, RED);
  delay(500);

  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, YELLOW);
  delay(500);

  digitalWrite(LED_GREEN, HIGH);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_GREEN_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, GREEN);
  delay(500);
}

void initLcd()
{
  g__gestion_lcd = new GestionLCD();
  g__gestion_lcd->init(100);
  g__gestion_lcd->Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 90, WHITE);
  g__gestion_lcd->Paint_SetRotate(90);
  g__gestion_lcd->clear(BLACK);
}

void setup()
{
  Serial.begin(115200);

  Serial.printf("\n\n");
  Serial.println(PROMPT);

  execForever();

  // Set timer frequency to 1Mhz
  g__timer = timerBegin(TIMER_FREQUENCY);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(g__timer, &onTimer);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(g__timer, TIMER_FREQUENCY / TIMER_RATIO, true, 0);

  pinMode(PIN_LED, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Initialisation du LCD
  initLcd();

  // Test Leds 
  testLeds();

  g__timers = new Timers();
  g__timers->start(TIMER_CHENILLARD, DURATION_TIMER_CHENILLARD, &callback_exec_chenillard);

  g__date_time = new DateTime();
  g__nmea = new Nmea();

  g__sdcard = new SDCard();

  g__button = new OneButton(PIN_INPUT, true);
  g__button->attachLongPressStart(callback_long_press_start, g__button);
  g__button->attachClick(callback_click, g__button);
  g__button->attachDoubleClick(callback_double_click, g__button);
  g__button->setLongPressIntervalMs(1000);

  //g__gestion_lcd->Paint_DrawString_EN(1, 8, PROMPT, &Font24, BLACK, WHITE);

#if USE_INCOMING_CMD
  memset(g__incoming_buff, '\0', sizeof(g__incoming_buff));
#endif
}

void loop()
{
  if (g__flg_expired_1ms == true) {
    g__nmea->readFrame();         // Lecture des trames NMEA

    noInterrupts();
    g__flg_expired_1ms = false;
    interrupts();
  }

  if (g__flg_expired_1_25ms == true) {
    // Emission alternee toutes les 2.5 mS (~4800 bauds) sur les 2 UART
    static bool g__flg_progress = false;

    if (g__flg_progress == false) {
      if (g__nmea->getInhSendTlv() == false) {
        g__nmea->sendTlv();
      }

      g__flg_progress = true;
    }
    else {
      // Provision pour le 2nd UART
      g__flg_progress = false;
    }

    noInterrupts();
    g__flg_expired_1_25ms = false;
    interrupts();
  }

  if (g__flg_expired_10ms) {
    g__timers->update();

    noInterrupts();
    g__flg_expired_10ms = false;
    interrupts();
  }

  if (g__flg_expired_500ms) {
    g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_BLUE_X, LIGHTS_POSITION_Y,
      (g__sdcard->getInhAppendGpsFrame() ? LIGHT_FULL_IDX : LIGHT_BORD_IDX), &Font24Symbols, BLACK, BLUE);


    noInterrupts();
    g__pass_on_timer = 0;
    g__flg_expired_500ms = false;
    interrupts();
  }

  g__timers->test();

  // Keep watching the push button:
  g__button->tick();

  g__nmea->gestionResponse();

  digitalWrite(LED_RED,    (g__state_leds & STATE_LED_RED)    ? LOW : HIGH);
  digitalWrite(LED_YELLOW, (g__state_leds & STATE_LED_YELLOW) ? LOW : HIGH);
  digitalWrite(LED_GREEN,  (g__state_leds & STATE_LED_GREEN)  ? LOW : HIGH);

#if USE_INCOMING_CMD
  gestionOfCommands();
#endif
}

#if USE_INCOMING_CMD
void usage_cmd()
{
	Serial.printf("Command:\n");
	Serial.printf("- PAINT\n");
	Serial.printf("- NMEA\n");
	Serial.printf("- SDCard\n");
}

void usage_paint()
{
	Serial.printf("Usage:\n");
	Serial.printf("- cache                             Dump du cache\n");
}

void usage_nmea()
{
	Serial.printf("Usage:\n");
	Serial.printf("- tlv enable/disable                Autorisation/Inhibition emission TLV\n");
	Serial.printf("- error                             Simulation erreur\n");
}

void usage_sdcard()
{
	Serial.printf("Usage:\n");
	Serial.printf("- init                              Initialisation\n");
	Serial.printf("- end                               Terminaison (permet une reprise apres les erreurs de la SD Card)\n");
	Serial.printf("- prepare                           Preparation\n");
	Serial.printf("- setInhAppendGpsFrame <flg>        Set/Reset inhibition of appending in 'GpsFrames.txt' (<flg> optional - true by default)\n");
	Serial.printf("- printInfos                        Informations (type et taille)\n");
	Serial.printf("- listdir <dir>                     Liste d'un repertoire donne (ie. /, /PILOT, etc.)\n");
	Serial.printf("- exists <path>                     Existence d'un repertoire ou d'un fichier\n");
	Serial.printf("- readFile <file>                   Lecture et impression d'un fichier donne (ie. /PILOT/GpsPilot.txt)\n");
	Serial.printf("- getFileLine <file> <nbr_lines>    Lectures successives d'une ligne d'un fichier\n");
	Serial.printf("- appendFile <file> <patterns>      Concatenation dans <file> avec des patterns constituant une ligne (ie. /LOG/log.txt ajout d'une ligne)\n");
	Serial.printf("- renameFile <path_from> <path_to>  Renommage d'un repertoire ou d'un fichier\n");
	Serial.printf("- deleteFile <file>                 Suppression d'un fichier\n");
	Serial.printf("- getFileLines [<file>]             Ouverture, lecture et analyse des enregistrements de <file> si non NULL; sinon 'NAME_OF_FILE_GPS_PILOT'\n");
}

void gestionOfCommands()
{
  static size_t   g__count = 0;
  static char     g__incoming_buff[256 + 1];   // +1 for '\0' terminal
  static bool     g__flg_wait_command = true;

  char l__copy_incomming_buff[sizeof(g__incoming_buff)];

  // Gestion of commands
  if (Serial.available() > 0) {
    // Read the incoming byte
    int incomingByte = Serial.read();

    if (incomingByte == '\n') {
      g__incoming_buff[g__count] = '\0';
      g__count = 0;

      Serial.print(">>> [");
      Serial.print(g__incoming_buff);
      Serial.printf("] (length: %d)\n", strlen(g__incoming_buff));

      if (!strncmp(g__incoming_buff, "PAINT", strlen("PAINT"))) {
      	strcpy(l__copy_incomming_buff, g__incoming_buff);

         char *l__pattern = strtok(l__copy_incomming_buff, " ");    // Skip "PAINT" command
         l__pattern = strtok(NULL, " ");                   // Get the sub-command
         if (l__pattern != NULL) {
         	if (!strcmp(l__pattern, "cache")) {            // 'cache' sub-command
            for (UWORD l__x = 0; l__x < LCD_X_SIZE_MAX; l__x++) {
              for (UWORD l__y = 0; l__y < LCD_Y_SIZE_MAX; l__y += 8) {

                char l__dump[8 + 1];
                memset(l__dump, '\0', sizeof(l__dump));

                for (UWORD l__idx = 0; l__idx < 8; l__idx++) {
                  // Presentation "naturelle" (inversion de 'x' et 'y')
                  l__dump[l__idx] = (g__gestion_lcd->Paint_GetCache(l__y + l__idx, l__x) == true) ? '#' : '.';
                }
                Serial.print(l__dump);
              }
              Serial.println();
            }
          }
          else {
            	Serial.printf("Unknown sub-command [%s]\n", l__pattern);
          	}
        }
        else {
			    usage_paint();
        }
      }
      else if (!strncmp(g__incoming_buff, "NMEA", strlen("NMEA"))) {
      	strcpy(l__copy_incomming_buff, g__incoming_buff);

         char *l__pattern = strtok(l__copy_incomming_buff, " ");    // Skip "NMEA" command
         l__pattern = strtok(NULL, " ");                   // Get the sub-command
         if (l__pattern != NULL) {
         	if (!strcmp(l__pattern, "error")) {            // 'error' sub-command
            	g__nmea->setError();
            }
         	else if (!strcmp(l__pattern, "tlv")) {         // 'tlv' sub-command
               l__pattern = strtok(NULL, " ");             // Get on/off
					bool l__flg =  g__nmea->getInhSendTlv();
               if (l__pattern != NULL) {
						if (!strcmp(l__pattern, "enable")) l__flg = false;
						else if (!strcmp(l__pattern, "disable")) l__flg = true;

						g__nmea->setInhSendTlv(l__flg);
              	}
					Serial.printf("\t=> TLV [%s]\n", (g__nmea->getInhSendTlv() == true) ? "No Send" : "Send");
            }
          	else {
            	Serial.printf("Unknown sub-command [%s]\n", l__pattern);
          	}
          }
          else {
				    usage_nmea();
          }
        }
      else if (!strncmp(g__incoming_buff, "SDCard", strlen("SDCard"))) {
      	strcpy(l__copy_incomming_buff, g__incoming_buff);

         char *l__pattern = strtok(l__copy_incomming_buff, " ");    // Skip "SDCard" command
         l__pattern = strtok(NULL, " ");                   // Get the sub-command
         if (l__pattern != NULL) {
         	if (!strcmp(l__pattern, "init")) {              // 'init' sub-command
            	bool l__flg_rtn = g__sdcard->init();
            	Serial.printf("\t=> init(): [%s]\n", (l__flg_rtn == true) ? "Ok" : "Ko");
            }
            else if (!strcmp(l__pattern, "end")) {              // 'end' sub-command
               g__sdcard->end();
               Serial.printf("\t=> end(): No rtn code\n");
            }
            else if (!strcmp(l__pattern, "prepare")) {        // 'end' sub-command
               g__sdcard->init(3);                           // 10 tentatives
               g__sdcard->callback_sdcard_retry_init_more();
            }
            else if (!strcmp(l__pattern, "printInfos")) {   // 'printInfos' sub-command
              bool l__flg_rtn = g__sdcard->printInfos();
              Serial.printf("\t=> printInfos(): [%s]\n", (l__flg_rtn == true) ? "Ok" : "Ko");
            }
            else if (!strcmp(l__pattern, "listDir")) {      // 'listDir' sub-command
               l__pattern = strtok(NULL, " ");               // Get the directory name
               const char *l__dir = "/";                     // Root by default
               if (l__pattern != NULL) {
						l__dir = l__pattern;
              	}
              	bool l__flg_rtn = g__sdcard->listDir(l__dir);
              	Serial.printf("\t=> listDir(%s): [%s]\n", l__dir, (l__flg_rtn == true) ? "Ok" : "Ko");
            }
            else if (!strcmp(l__pattern, "exists")) {        // 'exists' sub-command
              l__pattern = strtok(NULL, " ");                // Get the path name
              if (l__pattern != NULL) {
                const char *l__path = l__pattern;
                bool l__flg_rtn = g__sdcard->exists(l__path);
                Serial.printf("\t=> exists(%s): [%s]\n", l__path, (l__flg_rtn == true) ? "Ok" : "Ko");              
              }
              else {
                Serial.printf("\t=> exists(): Missing file name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "readFile")) {      // 'readFile' sub-command
              l__pattern = strtok(NULL, " ");                // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;
                bool l__flg_rtn = g__sdcard->readFile(l__file);
                Serial.printf("\t=> readFile(%s): [%s]\n", l__file, (l__flg_rtn == true) ? "Ok" : "Ko");              
              }
              else {
                Serial.printf("\t=> readFile(): Missing file name\n");
              }
            }
            else if (!strcmp(l__pattern, "getFileLine")) {   // 'getFileLine' sub-command
              l__pattern = strtok(NULL, " ");                // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;

                l__pattern = strtok(NULL, " ");               // Get the nbr of lines
                if (l__pattern != NULL) {
                  int l__nbr_lines = (int)strtol(l__pattern, NULL, 10);

                  for (int n = 0; n < l__nbr_lines; n++) {
                    String l__line = "";

                    bool l__flg_rtn = g__sdcard->getFileLine(
                      (n == 0) ? l__file : NULL,                  // Passage du nom du fichier sur la 1st demande
                      l__line,
                      (n == (l__nbr_lines - 1)) ? true : false);  // Cloture du fichier sur la derniere demande

                    Serial.printf("\t=> #%d: getFileLine(%s): [%s] [%s]\n", n, l__file, (l__flg_rtn == true) ? "Ok" : "Ko", l__line.c_str());

#if 0
                    // Si trame GPS, extraction du type 'I' suivi de DDMMYY (pour le renommage du fichier 'GpsFrames.txt')
                    if (!strncmp(l__line.c_str(), "AA", 2)) {
                      unsigned char l__cks_calculated = 0xff;
                      boolean l__cks_rtn = g__serial_nmea->calculChecksum((char *)l__line.c_str(), &l__cks_calculated);

                      Serial.printf("\t\t=> Cks [0x%02x] %s\n", l__cks_calculated, (l__cks_rtn == true ? "Ok" : "Ko"));

                      if (l__cks_rtn == true) {
                        char *l__pattern = strchr(l__line.c_str(), 'I');  // Type 'I' (date "I6DDMMYY" - 6 char)
                        if (l__pattern != NULL) {
                          char l__t_date[6+1];
                          memset(l__t_date, '\0', sizeof(l__t_date));
                          strncpy(l__t_date, l__pattern + 2, 6);          // Skip type and length fields

                          Serial.printf("\t\t\t=> Date [%s]\n", l__t_date);
                        }
                        else {
                          Serial.printf("\t\t\t=> No Date found\n");
                        }
                      }
                    }
#endif
                  }
                }
                else {
                  Serial.printf("\t=> getFileLine(): Missing nbr lines\n");
                }
              }
              else {
                Serial.printf("\t=> getFileLine(): Missing file name\n");
              }
            }
            else if (!strcmp(l__pattern, "appendFile")) {     // 'appendFile' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;
                String l__line = "";
                l__pattern = strtok(NULL, " ");               // Get the line
                while (l__pattern != NULL) {
                  l__line += l__pattern;
                  l__pattern = strtok(NULL, " ");             // Cont'd ...
                  if (l__pattern != NULL) {
                    l__line += " ";                           // ... with ' ' separator
                  }
                }
                l__line += "\n";                              // LF terminal for each line

                bool l__flg_rtn = g__sdcard->appendFile(l__file, l__line.c_str());
                Serial.printf("\t=> appendFile([%s], [%s]): [%s]\n", l__file, l__line.c_str(), (l__flg_rtn == true) ? "Ok" : "Ko");              
              }
              else {
                Serial.printf("\t=> appendFile(): Missing file name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "renameFile")) {     // 'renameFile' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the 1st path name
              if (l__pattern != NULL) {
                const char *l__path_from = l__pattern;
                l__pattern = strtok(NULL, " ");               // Get the 2nd path name
                if (l__pattern != NULL) {
                  const char *l__path_to = l__pattern;

                  bool l__flg_rtn = g__sdcard->renameFile(l__path_from, l__path_to);
                  Serial.printf("\t=> renamedFile([%s], [%s]): [%s]\n", l__path_from, l__path_to, (l__flg_rtn == true) ? "Ok" : "Ko");
                }
                else {
                  Serial.printf("\t=> renameFile(): Missing 2nd path name\n");                              
                }
              }
              else {
                Serial.printf("\t=> renameFile(): Missing 1st path name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "deleteFile")) {     // 'deleteFile' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;

                bool l__flg_rtn = g__sdcard->deleteFile(l__file);
                Serial.printf("\t=> deleteFile(%s): [%s]\n", l__file, (l__flg_rtn == true) ? "Ok" : "Ko");
              }
              else {
                Serial.printf("\t=> deleteFile(): Missing file name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "setInhAppendGpsFrame")) {      // 'setInhAppendGpsFrame' sub-command
              l__pattern = strtok(NULL, " ");                            // Get the flag (true/false pattern)
              if (l__pattern != NULL) {
                if (!strcmp(l__pattern, "false")) {
                  g__sdcard->setInhAppendGpsFrame(false);
                }
                else {
                  g__sdcard->setInhAppendGpsFrame(true);
                }   
              }
              else {
                g__sdcard->setInhAppendGpsFrame(true);
              }
            }
#if 0
            else if (!strcmp(l__pattern, "getFileLines")) {   // 'getFileLines' sub-command
              //char *l__file = NULL;
              l__pattern = strtok(NULL, " ");                 // Get the file name if exists
              if (l__pattern != NULL) {
                l__file = l__pattern;
              }
              g__file_gpspilot->getFileLines(l__file, true);
            }
#endif
          	else {
            	Serial.printf("Unknown sub-command [%s]\n", l__pattern);
          	}
          }
          else {
				    usage_sdcard();
          }
        }
        // Fin: Test de la SDCard
        else {
          Serial.printf("Unknown command [%s]\n", g__incoming_buff);
				usage_cmd();
        }

      g__flg_wait_command = true;
    }
    else if (g__count < sizeof(g__incoming_buff)) {
      g__incoming_buff[g__count++] = (char)incomingByte;
    }
    else {
      Serial.printf("Buffer overflow (%d >= %d)\n", g__count, sizeof(g__incoming_buff));
      g__count = 0;

      g__flg_wait_command = true;
    }
  }
  else if (g__flg_wait_command == true) {
    Serial.println(">>> Type the command...");
    g__count = 0;
    g__incoming_buff[g__count] = '\0';
    g__flg_wait_command = false;
  }
}
#endif    // USE_INCOMING_CMD

