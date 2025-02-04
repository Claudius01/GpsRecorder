// $Id: SDCard.cpp,v 1.5 2025/02/04 15:03:36 administrateur Exp $

#include <Arduino.h>

#include "SDCard.h"
#include "Misc.h"
#include "Timers.h"
#include "GestionLCD.h"

SPIClass spi1(HSPI); // Use the HSPI bus

SDCard::SDCard() : flg_init(false), nbr_of_init_retry(0),
                   cardType(CARD_NONE), cardSize((uint64_t)-1),
                   flg_sdcard_in_use(false), flg_inh_append_gps_frame(true),
                   m__gps_frame_size(0)
{
  Serial.println("SDCard::SDCard()");
}

SDCard::~SDCard()
{
  Serial.println("SDCard::~SDCard()");
}

/* Allumage Led en debut d'activite
 * => Car non retour dans 'loop()' pendant le temps de traitement de la methode ;-)
 */
void SDCard::startActivity()
{
  flg_sdcard_in_use = true;

  digitalWrite(STATE_LED_YELLOW, LOW);

  g__state_leds |= STATE_LED_YELLOW;
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, YELLOW);
}

/* 1 - Armement timer 'TIMER_SDCARD_ACCES' en fin d'activite
 *     => Extinction Led a l'expiration du timer (garantie d'une presentation minimale)
 * 2 - Allumage Led RED (via 'g__state_leds') + armement timer 'TIMER_SDCARD_ERROR' en fin d'activite si error
 *     => Extinction Led RED a l'expiration du timer (garantie d'une presentation minimale)
 */
void SDCard::stopActivity(boolean i__flg_no_error)
{
  g__timers->start(TIMER_SDCARD_ACCES, DURATION_TIMER_SDCARD_ACCES, &callback_end_sdcard_acces);

  if (i__flg_no_error == false) {
    g__state_leds |= STATE_LED_RED;
    g__timers->start(TIMER_SDCARD_ERROR, DURATION_TIMER_SDCARD_ERROR, &callback_end_sdcard_error);
    g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_SDC_RED_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, RED);
  }

  flg_sdcard_in_use = false;
}

bool SDCard::init()
{
  bool l__flg_rtn = false;

  startActivity();

  spi1.begin(SPI1_SCK, SPI1_MISO, SPI1_MOSI, SPI1_SS);
  spi1.setClockDivider(SPI_CLOCK_DIV2);

  if (!SD.begin(SPI1_SS, spi1)) {
    Serial.println("SDCard::init(): Error: SD Card mount failed");
  }
  else {
    flg_init = true;
    l__flg_rtn = true;
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

void SDCard::callback_sdcard_retry_init_more()
{
  if (init() == true) {
    flg_inh_append_gps_frame = false;

    Serial.printf("SDCard initialized\n");

    g__timers->stop(TIMER_SDCARD_RETRY_INIT);

    //  Preparation de la SDCard
    preparing();
  }
  else {
    end();

    if (nbr_of_init_retry > 0) {
      g__timers->start(TIMER_SDCARD_RETRY_INIT, DURATION_TIMER_SDCARD_RETRY_INIT, &callback_sdcard_retry_init);
      nbr_of_init_retry--;

      Serial.printf("\t#Initialization of SDCard (retry #%d)...\n", nbr_of_init_retry);
    }
    else {
      // Clignotement permanent de la Led RED
      g__timers->start(TIMER_SDCARD_INIT_ERROR, DURATION_TIMER_SDCARD_INIT_ERROR, &callback_sdcard_init_error);

      Serial.println("Initialization of SDCard (Fatal error)");
    }
  }
}

void SDCard::init(int i__nbr_retry)
{
  nbr_of_init_retry = i__nbr_retry;
  g__timers->start(TIMER_SDCARD_RETRY_INIT, DURATION_TIMER_SDCARD_RETRY_INIT, &callback_sdcard_retry_init);
}

void SDCard::end()
{
  startActivity();
  SD.end();
  stopActivity();
}

/*  Preparation de la SDCard
 *  - Renommage du fichier 'GpsFrames.txt' avec la datation de la 1st trame GPS
 *    => GPS [AA130910.000B1AC94847.0719D1NEA00157.5195F1EG40.11s40.20H5239.1I6040922K5124.1L1Ma212*3042] (Cks [0x42] Ok)
 *              => Extraction du type 'I' suivi de DDMMYY (ie. 040922)         ^^^^^^ -> 20220904 (YYYMMDD)
 *              => 'GpsFrames.txt' -> 'GpsFrames-20220904-1.txt' (1st suffixe -[0-9]* disponible - fichier inexistant) 
 */
bool SDCard::preparing()
{
  Serial.println("Preparing of SDCard...");

  bool l__flg_rtn = false;

  return l__flg_rtn;
}

/* Ouverture, concatenation et fermeture dans le fichier 'NAME_OF_FILE_GPS_FRAMES'
 */
bool SDCard::appendGpsFrame(const char *i__frame, boolean i__flg_force_append)
{
  /* Pour le test sur les longueurs ecrites a l'appel precedent
   * et fait avant une nouvelle concatenation
   * => Remarque: La longueur n'est pas maj apres de la concatenation 
   *              => Evite de reouvrir le fichier nouvellement ecrit ;-)
  */
  static size_t g__size_pre   = (size_t)-1;
  static size_t g__size_frame = (size_t)0;

  if (flg_init == false || flg_inh_append_gps_frame == true
   || (g__flg_inh_sdcard_ope == true && i__flg_force_append == false)) {
    return true;
  }

  bool l__flg_rtn = true;   // Operation reussie a priori

  startActivity();

  File file = SD.open(NAME_OF_FILE_GPS_FRAMES, FILE_APPEND);
  if (!file) {
    // Failed to open file for appending

#if 0
    Serial.printf("SDCard::appendGpsFrame(%s): Failed to open file for appending\n", NAME_OF_FILE_GPS_FRAMES);
#endif

    l__flg_rtn = false;
  }
  else {
    size_t l__size = file.size();

    if (g__size_pre != (size_t)-1) {
      if (l__size == (g__size_pre + g__size_frame)) {
#if 0
        // Trace de l'operation
        Serial.printf("SDCard::appendGpsFrame(%s): Write length: %d = (%d + %d) bytes\n",
          NAME_OF_FILE_GPS_FRAMES, l__size, g__size_pre, g__size_frame);
#endif
      }
      else {
#if 0
        // Trace de l'erreur
        Serial.printf("SDCard::appendGpsFrame(%s): Error length: %d != (%d + %d) bytes\n",
          NAME_OF_FILE_GPS_FRAMES, l__size, g__size_pre, g__size_frame);
#endif

        l__flg_rtn = false;
      }
    }

    g__size_pre = l__size;              // Longueur du fichier avant maj
    g__size_frame = strlen(i__frame);   // Longueur a ecrire sans le '\0' terminal ;-)

    if (!file.print(i__frame)) {
      // Line not appended
      l__flg_rtn = false;
    }
    else {
      m__gps_frame_size = (l__size + strlen(i__frame));
    }

    file.close();
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

// Methods for tests
bool SDCard::printInfos()
{
  bool l__flg_rtn = false;

  startActivity();

  if (flg_init == false) {
    Serial.println("SDCard::init(): Error: SD Card not initialized");
  }
  else {
    cardType = SD.cardType();

    if (cardType == CARD_NONE) {
      Serial.println("SDCard::init(): Error: No SD card attached");
    }
    else {
      Serial.print("SD Card Type: ");
      switch (cardType) {
      default: Serial.println("UNKNOWN");
        break;
      case CARD_MMC:
        Serial.println("MMC");
        l__flg_rtn = true;
        break;
      case CARD_SD:
        Serial.println("SDSC");
        l__flg_rtn = true;
        break;
      case CARD_SDHC:
        Serial.println("SDHC");
        l__flg_rtn = true;
        break;
      }
    }

    cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %llu MBytes\n", cardSize);
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

bool SDCard::listDir(const char *i__dir)
{
  bool l__flg_rtn = false;

  startActivity();

  File root = SD.open(i__dir);
  if (!i__dir) {
    // Failed to open directory
     Serial.printf("SDCard::listDir(NULL): Failed to open directory\n");
  }
  else {
    if (!root.isDirectory()) {
      // Not a directory
      Serial.printf("SDCard::listDir(%s): Not a directory\n", i__dir);
    }
    else {
      File file = root.openNextFile();
      while (file) {
        if (file.isDirectory()) {
          Serial.print("  Dir: [");
          Serial.print(file.name());
          Serial.print("]\n");
        }
        else {
          Serial.print("  File: [");
          Serial.print(file.name());
          Serial.print("] Size: [");
          Serial.print(file.size());
          Serial.print("]\n");
        }

        file = root.openNextFile();
      }

      l__flg_rtn = true;
    }
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

bool SDCard::exists(const char *i__path)
{
  bool l__flg_rtn = false;

  startActivity();

  l__flg_rtn = SD.exists(i__path);

  stopActivity();

  return l__flg_rtn;
}

bool SDCard::readFile(const char *i__file)
{
  bool l__flg_rtn = false;

  startActivity();

  File file = SD.open(i__file);
  if (!file) {
    // Failed to open file for reading
    Serial.printf("SDCard::readFile(%s): Failed to open file for reading\n", i__file);
  }
  else {
    while (file.available()) {
      int l__value = file.read();
      Serial.write(l__value);
    }

    file.close();

    l__flg_rtn = true;
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

bool SDCard::getFileLine(const char *i__file, String &o__line, bool i__flg_close)
{
  /* Non initialization ('open' retourne une classe 'File' !..)
   * => TODO: Accueillir une structure avec un attribut 'flg_avalaible'
   *    => Permettra d'ouvrir plusieurs fichiers simultanement et utiliser le 'flush' / 'close'
   */
  static File g__file_line;

  bool l__flg_rtn = false;

  startActivity();

  if (i__file != NULL) {
    g__file_line = SD.open(i__file);

    if (!g__file_line) {
      // Failed to open file for reading line
      Serial.printf("SDCard::getFileLine(%s): Failed to open file for reading line\n", i__file);
    }
  }

  if (g__file_line) {
    while (g__file_line.available()) {
      int l__value = g__file_line.read();

      if (l__value != '\r') {
        if (l__value != '\n') {
          o__line.concat((char)l__value);
        }
        else {
          l__flg_rtn = true;    // Retour 'Ok' si la ligne est effectivement lue
          break;
        }
      }
    }

    if (i__flg_close == true) {
      g__file_line.close();
    }
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

/* Ouverture, concatenation et fermeture dans le fichier passe en argument
 */
bool SDCard::appendFile(const char *i__file, const char *i__line)
{
  bool l__flg_rtn = false;

  startActivity();

  File file = SD.open(i__file, FILE_APPEND);
  if (!file) {
    // Failed to open file for appending
    Serial.printf("SDCard::appendFile(%s): Failed to open file for appending\n", i__file);
  }
  else {
    if (file.print(i__line)) {
      // Line appended
      l__flg_rtn = true;
    }

    file.flush();        // TBC: A tester sans le 'close()' ;-)
    file.close();
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

bool SDCard::renameFile(const char *i__path_from, const char *i__path_to)
{
  bool l__flg_rtn = false;

  startActivity();

  if (SD.rename(i__path_from, i__path_to)) {
    l__flg_rtn = true;
  }
  else {
    Serial.printf("SDCard::renameFile([%s], [%s]): Failed\n", i__path_from, i__path_to);
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

bool SDCard::deleteFile(const char *i__path)
{
  bool l__flg_rtn = false;

  startActivity();

  if (SD.remove(i__path)) {
    l__flg_rtn = true;
  }
  else {
    Serial.printf("SDCard::deleteFile(%s): Failed\n", i__path);
  }

  stopActivity(l__flg_rtn);

  return l__flg_rtn;
}
// End: Methods for tests

/* Formatage d'une taille; ie:
   -     56b (chgt 1 byte)
   -   1.8Kb (chgt 100 bytes -> ~ trame TLV)
   - 750.5Kb (chgt 100 bytes -> ~ trame TLV)
   - 2.567Mb (chgt  1 Kbytes -> ~ 10 trames TLV)
*/
void SDCard::formatSize(size_t i__value, char *o__buffer)
{
#define SIZE_1_KILO_BYTE        1000
#define SIZE_1_MEGA_BYTE     1000000
#define SIZE_10_MEGA_BYTE   10000000

  if (i__value < SIZE_10_MEGA_BYTE) {
    if (i__value < SIZE_1_KILO_BYTE) sprintf(o__buffer, "%u b", i__value);
    else if (i__value < SIZE_1_MEGA_BYTE) sprintf(o__buffer, "%u.%02u Kb", (i__value / SIZE_1_KILO_BYTE), (i__value % SIZE_1_KILO_BYTE) / 100);
    else sprintf(o__buffer, "%u.%03u Mb", (i__value / SIZE_1_MEGA_BYTE), (i__value % SIZE_1_MEGA_BYTE) / 1000);
  }
  else {
    sprintf(o__buffer, "> 10 Mb");
  }
}
