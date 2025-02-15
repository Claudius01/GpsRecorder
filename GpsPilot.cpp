// $Id: GpsPilot.cpp,v 1.16 2025/02/12 14:10:20 administrateur Exp $

#include <Arduino.h>

#include "Misc.h"
#include "Timers.h"
#include "DateTime.h"
#include "GestionLCD.h"
#include "Nmea.h"
#include "SDCard.h"
#include "GpsPilot.h"

void callback_send_gpspilot()
{
  g__gps_pilot->send();

  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, YELLOW);
}

GpsPilot::GpsPilot() : m__progress(PROGRESS_SEND_IDLE), m__buffer_idx(0), m__buffer_to_send("")
{
  Serial.println("GpsPilot::GpsPilot()");

  resetProperties();

  calculChecksumIncClearValues();

  m__properties.file_name = GPSPILOT_FILE_NAME;
  m__properties.name_trace = "";
  m__properties.distance = "";
  m__properties.cumul_ele_pos = "";
  m__properties.cumul_ele_neg = "";
}

GpsPilot::~GpsPilot()
{
  Serial.println("GpsPilot::~GpsPilot()");
}

void GpsPilot::resetProperties()
{
  m__properties.buffer_num = 0;
  m__properties.size_file = 0;
  m__properties.size_read = 0;
  m__properties.idx_line = 0;

  m__properties.cks_value = 0;
  m__properties.cks_size = 0;
}

void GpsPilot::prepareBufferToSend(const char *i__text)
{
  m__buffer_idx = 0;
  m__buffer_to_send = "";

  if (i__text != NULL) {
    m__buffer_to_send.concat(i__text);
    m__properties.buffer_num++;
  }
}

/* Emission cadencee du fichier '/PILOT/GpsPilot.txt'
   -    Methode lancee par 'GpsPilot send'
   - ou A l'expiration du timer ''
*/
void GpsPilot::send()
{
  switch (m__progress) {
  case PROGRESS_SEND_IDLE:
    m__progress = PROGRESS_SEND_HEADER_IN_PROGRESS;

    // Emission de 'TEXT_GPS_PILOT_UNO_HEADER'
    m__properties.buffer_num = 0;
    prepareBufferToSend(TEXT_GPS_PILOT_UNO_HEADER);
    break;

  case PROGRESS_SEND_HEADER_DONE:
    // Emission de 'TEXT_GPS_PILOT_UNO_BEGIN'
    m__progress = PROGRESS_SEND_BEGIN_IN_PROGRESS;

    prepareBufferToSend(TEXT_GPS_PILOT_UNO_BEGIN);
    m__properties.idx_line = 0;
    break;

  case PROGRESS_SEND_BEGIN_DONE:
  case PROGRESS_SEND_DATAS_IN_PROGRESS:
    m__progress = PROGRESS_SEND_DATAS_IN_PROGRESS;  

    {
      // Ouverture et lecture ligne a ligne du fichier 'GpsPilot.txt'
      bool l__end_of_file = false;
      String l__line = "";
      bool l__flg_rtn = readFile(l__line, &l__end_of_file);

      if (l__flg_rtn == true) {
        prepareBufferToSend(l__line.c_str());

        // Prises des token '#Name' et '#Distance'
        char *l__token = NULL;
        if ((l__token = strstr(l__line.c_str(), "#Name ")) != NULL) {
          getTokenStrValue((l__token + strlen("#Name ")), m__properties.name_trace);
        }
        else if ((l__token = strstr(l__line.c_str(), "#Distance ")) != NULL) {
          getTokenNumValue((l__token + strlen("#Distance ")), m__properties.distance);
        }
        else if ((l__token = strstr(l__line.c_str(), "#Cumul Elevation Positive ")) != NULL) {
          getTokenNumValue((l__token + strlen("#Cumul Elevation Positive ")), m__properties.cumul_ele_pos);
        }
        else if ((l__token = strstr(l__line.c_str(), "##Cumul Elevation Negative ")) != NULL) {
          getTokenNumValue((l__token + strlen("#Cumul Elevation Negative ")), m__properties.cumul_ele_neg);
        }
        else if ((l__token = strstr(l__line.c_str(), "#Checksum ")) != NULL) {
          getTokenCksValue((l__token + strlen("#Checksum ")));
        }
        // Fin: Prises des token '#Name' et '#Distance'

        if (l__end_of_file == false) {
          m__properties.idx_line++;

          // Checksum incrementale sans l'enregistrement "#Checksum..."
          l__line.concat('\n');     // Add '\n' terminal
          calculChecksumInc((uint8_t *)l__line.c_str(), l__line.length());
        }
        else {
          m__progress = PROGRESS_SEND_DATAS_LAST_LINE;

          // Checksum finale
          calculChecksumInc(NULL, 0, true);
  
          Serial.printf("\tCks [%lu] [0x%08lx] Size [%u] (reading)\n",
            m__properties.cks_value, m__properties.cks_value, m__properties.cks_size);

          Serial.printf("\tCks [%lu] [0x%08lx] Size [%u] (calculated)\n",
            m__calc_cks.checksum_calc, m__calc_cks.checksum_calc, m__calc_cks.size_datas_calc);

          if (m__properties.cks_value != m__calc_cks.checksum_calc || m__properties.cks_size != m__calc_cks.size_datas_calc) {
            Serial.printf("GpsPilot::send(): Wrong Cks r/c (read [%lu] != calc [%lu]) a/o Size (read [%u] != calc [%u])\n",
            m__properties.cks_value, m__calc_cks.checksum_calc, m__properties.cks_size, m__calc_cks.size_datas_calc);

            g__gestion_lcd->Paint_DrawString_EN(6, 24, m__properties.name_trace.c_str(), &Font16, BLACK, RED);
            g__gestion_lcd->Paint_DrawString_EN(6, 40, m__properties.distance.c_str(), &Font16, BLACK, RED);
            g__gestion_lcd->Paint_DrawString_EN(6 + (11 * 12), 40, "+/- ... M", &Font16, BLACK, RED);

            /* TODO: Maj partielle des infos            1         2
                                              012345678901234567890
                                             "uu.d Km     +/- ... M"
            */
            // Effacement du noms et de la distance lus depuis 'GpsPilot.txt'
            m__properties.name_trace = "";
            m__properties.distance = "";
            m__properties.cumul_ele_pos = "";
            m__properties.cumul_ele_neg = "";
          }
          else {
            g__gestion_lcd->Paint_DrawString_EN(6, 24, m__properties.name_trace.c_str(), &Font16, BLACK, GREEN);
            g__gestion_lcd->Paint_DrawString_EN(6, 40, m__properties.distance.c_str(), &Font16, BLACK, GREEN);
            g__gestion_lcd->Paint_DrawString_EN(6 + (11 * 12), 40, "+/- ... M", &Font16, BLACK, GREEN);
          }
        }
      }
      else {
        /* Arret avec emission du texte 'TEXT_GPS_PILOT_ERROR'
           qui provoquera une erreur a la reception
        */
        m__progress = PROGRESS_SEND_DATAS_LAST_LINE;
        prepareBufferToSend(TEXT_GPS_PILOT_ERROR);
      }
    }
    break;

  case PROGRESS_SEND_DATAS_DONE:
    // Emission de 'TEXT_GPS_PILOT_UNO_END'
    m__progress = PROGRESS_SEND_END_IN_PROGRESS;

    prepareBufferToSend(TEXT_GPS_PILOT_UNO_END);
    break;

  case PROGRESS_SEND_END_DONE:
    /* Fin de l'emission complete du fichier 'GpsPilot.txt' et de l'habillage
      => Reinitialisation pour permettre une action "GpsPilot send" dans le 'Serial Monitor'
    */
    m__progress = PROGRESS_SEND_IDLE;

    m__buffer_idx = 0;
    m__buffer_to_send = "";
  
    resetProperties();
  
    calculChecksumIncClearValues();

    // Armement timer 'TIMER_ACTIVATE_SDCARD'
    g__timers->start(TIMER_ACTIVATE_SDCARD, DURATION_TIMER_ACTIVATE_SDCARD, &callback_activate_sdcard);
    break;

  default:
    Serial.printf("GpsPilot::send()): Internal error: State [%d] not expected\n", m__progress);
    break;
  }
}

void GpsPilot::sendBuffer()
{
  if (m__buffer_idx == 0) {
    Serial.printf("#%u: [", (m__properties.buffer_num - 1));
  }

  if (m__buffer_to_send.length() != 0) {
    char l__char = m__buffer_to_send.charAt(m__buffer_idx++);

    Serial.printf("%c", l__char);

    g__nmea->sendChar(l__char);   // Emission du caractere
  }

  if (m__buffer_idx == m__buffer_to_send.length()) {
    // Le buffer a ete entierement emis au caractere '\n' pres
    Serial.printf("]\n");

    g__nmea->sendChar('\n');      // Emission fin de ligne

    // Vidage du buffer emis
    prepareBufferToSend(NULL);
  
    // Passage a l'etat suivant 'DONE' sauf dans le cas 'PROGRESS_SEND_DATAS_IN_PROGRESS'
    switch (m__progress) {
    case PROGRESS_SEND_HEADER_IN_PROGRESS: m__progress = PROGRESS_SEND_HEADER_DONE; break;
    case PROGRESS_SEND_BEGIN_IN_PROGRESS:  m__progress = PROGRESS_SEND_BEGIN_DONE; break;
    case PROGRESS_SEND_DATAS_IN_PROGRESS: break;
    case PROGRESS_SEND_DATAS_LAST_LINE:    m__progress = PROGRESS_SEND_DATAS_DONE; break;
    case PROGRESS_SEND_END_IN_PROGRESS:    m__progress = PROGRESS_SEND_END_DONE; break;
    default:
      Serial.printf("GpsPilot::sendBuffer(): Internal error: State [%d] not expected\n", m__progress);
      break;
    }

    // Armement du timer 'TIMER_SEND_GPSPILOT'
    g__timers->start(TIMER_SEND_GPSPILOT, DURATION_TIMER_SEND_GPSPILOT, &callback_send_gpspilot);
    g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, YELLOW);
  }
}

bool GpsPilot::readFile(String &o__line, bool *o__end_of_file)
{
  bool l__flg_rtn = false;

  // Determination de la taille du fichier si pas fait
  if (m__properties.size_file == 0) {
    m__properties.size_file = g__sdcard->sizeFile(m__properties.file_name);
    m__properties.size_read = 0;
  }

  if (m__properties.size_file != -1) {
    /* Passage du nom du fichier sur la 1st lecture
       => 'l__line' est depossede du '\n' terminal
    */
    size_t l__size = g__sdcard->readFileLine((m__properties.idx_line == 0 ? m__properties.file_name : NULL), o__line);

    if (l__size != -1) {
      m__properties.size_read += (l__size + 1);    // +1 for '\n'
      l__flg_rtn = true;
    }

#if 0
    Serial.printf("\t=> #%d: GpsPilot::readFile(%s): [%s] [%s] (%u bytes) (%u bytes read)\n",
      m__properties.idx_line, m__properties.file_name, (l__flg_rtn == true) ? "Ok" : "Ko", o__line.c_str(), l__size, m__properties.size_read);
#endif

    if (l__flg_rtn == false) {
      Serial.printf("\t=> readFileLine(%s): Error read\n", m__properties.file_name);
      l__flg_rtn = false;
    }

    // Cloture et arret si totalite du fichier lu
    if (m__properties.size_read == m__properties.size_file) {
      g__sdcard->readFileLine(NULL, o__line, true);

#if 0
      Serial.printf("\t=> readFileLine(%s): End of read (Ok)\n", m__properties.file_name);
#endif

      *o__end_of_file = true;
      l__flg_rtn = true;
    }

    // Cloture et arret si depassement du fichier lu
    else if (m__properties.size_read > m__properties.size_file) {
      String l__line = "";
      g__sdcard->readFileLine(NULL, l__line, true);

      Serial.printf("\t=> readFileLine(%s): End of read (Ko)\n", m__properties.file_name);
      l__flg_rtn = false;
    }
  }
  else {
    Serial.printf("\t=> readFileLine(%s): Error read size\n", m__properties.file_name);
    l__flg_rtn = false;
  }

  return l__flg_rtn;
}

void GpsPilot::getTokenStrValue(const char *i__text, String &o__propertie)
{
  char *l__text = strdup(i__text);

  // Suppression des '[' et ']'
  char *l__pattern1 = strchr(l__text, '[');
  char *l__pattern2 = strchr(l__text, ']');

  if (l__pattern1 != NULL && l__pattern2 != NULL) {
    *l__pattern2 = '\0';

    //Serial.printf("GpsPilot::getTokenStrValue {%s}\n", (l__pattern1 + 1));

    /* Troncature si > 21 caracteres d'une ligne
       => ie. "Dim. 09/02/2025 16:52"
    */
    char l__buffer[32];
    memset(l__buffer, '\0', sizeof(l__buffer));
    strncpy(l__buffer, (l__pattern1 + 1), 21);

    o__propertie = l__buffer;

    g__gestion_lcd->Paint_DrawString_EN(6, 24, l__buffer, &Font16, BLACK, YELLOW);
  }

  free(l__text);
}

void GpsPilot::getTokenNumValue(const char *i__text, String &o__propertie)
{
  char *l__text = strdup(i__text);

    // Suppression des '[' et ']'
  char *l__pattern1 = strchr(l__text, '[');
  char *l__pattern2 = strchr(l__text, ']');

  if (l__pattern1 != NULL && l__pattern2 != NULL) {
    *l__pattern2 = '\0';

    unsigned int l__value = (unsigned int)strtol((l__pattern1 + 1), NULL, 10);

    //Serial.printf("GpsPilot::getTokenNumValue {%u} M\n", l__value);

    char l__buffer[32];
    memset(l__buffer, '\0', sizeof(l__buffer));
    sprintf(l__buffer, "%u.%u Km", l__value / 1000, (l__value % 1000) / 100);

    o__propertie = l__buffer;

    // TODO: Sortir la presentation ;-)
    g__gestion_lcd->Paint_DrawString_EN(6, 40, l__buffer, &Font16, BLACK, YELLOW);
  }

  free(l__text);
}

void GpsPilot::getTokenCksValue(const char *i__text)
{
  char *l__text = strdup(i__text);

    // Suppression des '[' et ']'
  char *l__pattern1 = strchr(l__text, '[');
  char *l__pattern2 = strchr(l__text, ']');

  if (l__pattern1 != NULL && l__pattern2 != NULL) {
    *l__pattern2 = '\0';

    Serial.printf("GpsPilot::getTokenCksValue {%s}\n", (l__pattern1 + 1));

    // Extraction de la checksum et de la taille
    char *l__text_dup = strdup(l__pattern1 + 1);
    char *l__pattern = NULL;
    l__pattern = strtok(l__text_dup, " \t");
    if (l__pattern != NULL) {
      m__properties.cks_value = (uint32_t)strtoul(l__pattern, NULL, 10);
      Serial.printf("\tCks [%s] (%lu)\n", l__pattern, m__properties.cks_value);

      l__pattern = strtok(NULL, " \t");
      if (l__pattern != NULL) {
        m__properties.cks_size = (size_t)strtol(l__pattern, NULL, 10);
        Serial.printf("\tSize [%s] (%u)\n", l__pattern, m__properties.cks_size);
      }
    }
    free(l__text_dup);
    // Fin: Extraction de la checksum et de la taille
  }

  free(l__text);
}

void GpsPilot::calculChecksumInc(const uint8_t *i__datas, size_t i__size, boolean i__flg_end)
{
  if (i__flg_end == false) {
    m__calc_cks.size_cumul += i__size;
    m__calc_cks.crc_tmp = cksum_inc(i__datas, i__size, m__calc_cks.crc_tmp, 0);
  }
  else {
    m__calc_cks.checksum_calc = cksum_inc(NULL, 0, m__calc_cks.crc_tmp, m__calc_cks.size_cumul);
    m__calc_cks.size_datas_calc = m__calc_cks.size_cumul;

    // Prepare for next call
    m__calc_cks.size_cumul = 0;
    m__calc_cks.crc_tmp = 0;
  }
}

void GpsPilot::calculChecksumIncClearValues()
{
  m__calc_cks.checksum_calc = (uint32_t)-1;
  m__calc_cks.size_datas_calc = 0;
  m__calc_cks.crc_tmp = 0;
  m__calc_cks.size_cumul = 0;
}
