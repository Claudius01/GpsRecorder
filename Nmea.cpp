// $Id: Nmea.cpp,v 1.13 2025/02/04 15:03:36 administrateur Exp $

/* Gestion des trames NMEA a 4800 bauds

   - Cible: ESP32-S3-GEEK (ESP32S3 Dev Module + USB CDC On Boot: Enabled)
   - Rx: GPIO14
   - Tx: GPIO13

   - Traitements identiques au GpsRecorder implementes sur un Arduino UNO + Module GPS EM 406A
     => Les enregistrements sont effectues sur la SDCard
*/

#include <HardwareSerial.h>
#include <Arduino.h>

#include "Timers.h"
#include "DateTime.h"
#include "GestionLCD.h"
#include "Nmea.h"
#include "SDCard.h"

#define USE_TRACE_NMEA            0
#define USE_TRACE_NMEA_CONTENT    0
#define USE_TRACE_TLV             0

static HardwareSerial   g__serial_nmea(1);

/* Connexion suite a la reception d'une nouvelle trame NMEA
*/ 
void callback_exec_deconnexion()
{
  g__nmea->setConnected(false);

  g__gestion_lcd->Paint_DrawString_EN(6, 8, g__nmea->getDateTimeLcd(), &Font16, BLACK, YELLOW);
}

void callback_nmea_end_activity()
{
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, YELLOW);
}

void callback_nmea_end_error()
{
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_RED_X, LIGHTS_POSITION_Y, LIGHT_BORD_IDX, &Font24Symbols, BLACK, RED);
}

Nmea::Nmea()
{
  Serial.println("Nmea::Nmea()");

  m__idx_frames = 0;
  m__size_buff_rec = 0;
  memset(m__buffer_rec, '\0', sizeof(m__buffer_rec));

  m__idx_tlv = 0;
  memset(m__buffer_tlv, '\0', sizeof(m__buffer_tlv));

  m__idx_send = 0;
  m__pos_char_send = 0;
  memset(m__buffer_send, '\0', sizeof(m__buffer_send));

  memset(&m__infos_gps, '\0', sizeof(m__infos_gps));

  pinMode(NMEA_RXD, INPUT_PULLUP);
  pinMode(NMEA_TXD, OUTPUT);

  g__serial_nmea.begin(4800, SERIAL_8N1, NMEA_RXD, NMEA_TXD);

  m__connected = false;

  m__infos_gps.flg_inh_send_tlv = true;
}

Nmea::~Nmea()
{
  Serial.println("Nmea::~Nmea()");
}

void Nmea::readFrame()
{
  char l__char = '\0';

  while (g__serial_nmea.available() > 0) {
    l__char = (char)g__serial_nmea.read();

    if (l__char == '\r' || l__char == '\n') {
      // Buffer complet
      if (m__size_buff_rec > 0) {
        m__buffer_rec[m__size_buff_rec] = '\0';

        bool l__flg_gprmc = !strncmp(m__buffer_rec, "$GPRMC", 6) ? true : false;
        bool l__flg_gpgga = !strncmp(m__buffer_rec, "$GPGGA", 6) ? true : false;

        if (l__flg_gprmc == true || l__flg_gpgga == true) {
          // Calcul of checksum only with 'GPRMC' and 'GPGGA' frames
          unsigned char l__cks_calculated = 0x00;
          unsigned char l__cks_received   = 0x00;

          bool l__flg_rtn = calculChecksum(m__buffer_rec, &l__cks_calculated, &l__cks_received);

#if USE_TRACE_NMEA
          Serial.printf("#%u: [%s] (%d bytes) (%s)\n",
            m__idx_frames++, m__buffer_rec, strlen(m__buffer_rec), (l__flg_rtn == true) ? "Ok" : "Ko");
#endif

          if (l__flg_rtn == true) {
            if (l__flg_gprmc == true) {
              extractInfosGPRMC(m__buffer_rec);
            }
            else if (l__flg_gpgga == true) {
              extractInfosGPGGA(m__buffer_rec);
            }
          }
          else {
            Serial.printf("Nmea::readChar(): Wrong Cks [calc: 0x%02x] [rec: 0x%02x]\n", l__cks_calculated, l__cks_received);
            setError();
          }
        }
#if 0
        else {
          Serial.printf("[%s] (%d bytes)\n", m__buffer_rec, strlen(m__buffer_rec));
        }
#endif
      }

      m__size_buff_rec = 0;
    }
    else if (m__size_buff_rec < (sizeof(m__buffer_rec) - 1)) {
      m__buffer_rec[m__size_buff_rec++] = l__char;
    }
    else {
      Serial.printf("Nmea::readChar(): Too many char (%u bytes)\n", m__size_buff_rec);
      setError();

      m__size_buff_rec = 0;
    }
  }
}

bool Nmea::calculChecksum(char *i__frame, unsigned char *o__cks_calculated, unsigned char *o__cks_expected)
{
  bool l__cks_end = false;
  size_t l__size = strlen(i__frame);

  // If 'o__cks_expected != NULL'; Start calcul after '$' character
  // Else; Start calcul with all characters
  size_t n = (o__cks_expected != NULL) ? 1 : 0;

  for (; n < l__size; n++) { 
    char l__byte = *(i__frame + n);
    if (l__byte == '*') {
      l__cks_end = true;
    }
    else if (l__cks_end) {
      // Get and convert in hexa the 2 last characters after '*' character
      if (n == (l__size - 2) && o__cks_expected != NULL) {
        *o__cks_expected = (unsigned char)strtol(&i__frame[n], NULL, 16);
      }
    }
    else {
      // Calculate the checksum between {'$', ..., '*'}
      *o__cks_calculated ^= (unsigned char)l__byte;
    }
  }

  if (o__cks_expected != NULL) {
    return (*o__cks_calculated == *o__cks_expected) ? true : false;
  }
  else {
    return true;
  }
}

/* Conversion ascii des knots en kmh
  => Code inspire de celui implemente sur ATmega328P
     => Non utilisation des 'float' ;-)
        => TODO: Use the 'float' 
*/
void Nmea::convertSpeedKnotsToKmH()
{
	char l__buff_work[16];
	memset(l__buff_work, '\0', sizeof(l__buff_work));
	strncpy(l__buff_work, m__infos_gps.t_speedKnots, min(sizeof(m__infos_gps.t_speedKnots) - 1, sizeof(l__buff_work) - 1));
	strcat(l__buff_work, "00");	// Accueil pour la x100

	char *l__sep = strchr(l__buff_work, '.');
	if (l__sep != NULL) {
		// La vitesse "ascii" est multilpliee par 100
		*l__sep       = *(l__sep + 1);
		*(l__sep + 1) = *(l__sep + 2);
		*(l__sep + 2) = '\0';

		// La vitesse numerique est multilpliee par 1000
		long l__speed_kmh = 1852L * strtol(l__buff_work, NULL, 10);

		sprintf(l__buff_work, "%015ld", l__speed_kmh);

		/* Extraction...
		 * ie. #   26 - [speed [0.70] knots]
		 *     #   27 - [speed [129640] kmh]
		 *     => - extraction de "29640" avec seulement "29"
		 *     => - extraction de "1"
		 */
		char l__speedKmH_d[3];
		strncpy(l__speedKmH_d, &l__buff_work[strlen(l__buff_work) - 5], 2);
		l__speedKmH_d[2] = '\0';
		l__buff_work[strlen(l__buff_work) - 5] = '\0';

		sprintf(m__infos_gps.t_speedKmH, "%d.%s", (int)strtol(l__buff_work, NULL, 10), l__speedKmH_d);
	}
	else {
		strcpy(m__infos_gps.t_speedKmH, "0.0");
	}
}

void Nmea::extractInfosGPRMC(char *i__frame)
{
	char *l__pattern = NULL;

	int l__idx = 0;
	l__pattern = strtok(i__frame, ",");   // TBC: Test avec champs "absent" entre ",,"
	while (l__pattern != NULL) {
		switch (l__idx) {
		case 1: // Time: "hhmmss\0"
			strncpy(m__infos_gps.t_time, l__pattern, 6);
			m__infos_gps.t_time[6] = '\0';
			m__infos_gps.flg_infos |= INFO_GPS_TIME;

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\ttime [%s]\n", m__infos_gps.t_time);
#endif

			// Détection des secondes [0, 5, 10, ..., 55] pour l'émission de la réponse
			if (m__infos_gps.t_time[5] == '0' || m__infos_gps.t_time[5] == '5') {
				m__infos_gps.flg_prepare_tlv = true;
			}
		break;

		case 2: // status
			m__infos_gps.status = *l__pattern;
			m__infos_gps.flg_infos |= INFO_GPS_STATUS;

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tstatus [%c]\n", m__infos_gps.status);
#endif
			break;

		case 3: // Latitude
			strncpy(m__infos_gps.t_latitude, l__pattern, sizeof(m__infos_gps.t_latitude));

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tlat [%s] degrees", m__infos_gps.t_latitude);
#endif
			m__infos_gps.flg_infos |= INFO_GPS_LAT;
			break;

		case 4: // North/South
 			if (*l__pattern == 'N') {
		 		m__infos_gps.north_south = GPS_NORTH;
	 			m__infos_gps.flg_infos |= INFO_GPS_LAT_DIR;
 			}
 			else if (*l__pattern == 'S') {
	 			m__infos_gps.north_south = GPS_SOUTH;
	 			m__infos_gps.flg_infos |= INFO_GPS_LAT_DIR;
 			}

#if USE_TRACE_NMEA_CONTENT
			Serial.printf(" [%s]\n", l__pattern);
#endif
			break;

		case 5: // Longitude
			strncpy(m__infos_gps.t_longitude, l__pattern, sizeof(m__infos_gps.t_longitude));

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tlon [%s] degrees", m__infos_gps.t_longitude);
#endif
			m__infos_gps.flg_infos |= INFO_GPS_LON;
			break;

		case 6: // East/West
			if (*l__pattern == 'E') {
				m__infos_gps.east_west = GPS_EAST;
				m__infos_gps.flg_infos |= INFO_GPS_LON_DIR;
			}
			else if (*l__pattern == 'W') {
				m__infos_gps.east_west = GPS_WEST;
				m__infos_gps.flg_infos |= INFO_GPS_LON_DIR;
			}

#if USE_TRACE_NMEA_CONTENT
			Serial.printf(" [%s]\n", l__pattern);
#endif
			break;

		case 7: // Speed in Knots and KmH
			strncpy(m__infos_gps.t_speedKnots, l__pattern, sizeof(m__infos_gps.t_speedKnots));

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tspeed [%s] knots\n", m__infos_gps.t_speedKnots);
#endif
			convertSpeedKnotsToKmH();

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tspeed [%s] kmh\n", m__infos_gps.t_speedKmH);
#endif
			m__infos_gps.flg_infos |= INFO_GPS_SPEED;
			break;

		case 8: // Cap
			strncpy(m__infos_gps.t_cap, l__pattern, sizeof(m__infos_gps.t_cap));

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tcap [%s] degrees\n", m__infos_gps.t_cap);
#endif
			m__infos_gps.flg_infos |= INFO_GPS_CAP;
			break;

		case 9: // Date: "mmddyy\0"
			strncpy(m__infos_gps.t_date, l__pattern, 6);
			m__infos_gps.t_date[6] = '\0';
			m__infos_gps.flg_infos |= INFO_GPS_DATE;

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tdate [%s]\n", m__infos_gps.t_date);
#endif
			break;

		default:
			break;
		}
    
		l__pattern = strtok(NULL, ",");
		l__idx++;
	}

  /* Determination de l'état 'CONNECTED' suite à:
   *    - l'info. 'Availaible' de la trame GPRMC
   *    - l'inrémentation de +1 du champ 'time' de la trame GPRMC
   *      Remarque: Seule les HH:MM:SS sont prises en compte; le passage à la minute suivante (ex: 07:23:59 => 07:24:00)
   *                fera que l'état 'CONNECTED' ne sera pas m.a.j. pendant 1 Sec. (< au 'time-out' de 3 Sec.
   *                avant de passer en 'NOT_CONNECTED')
   */
  if ((m__infos_gps.flg_infos & INFO_GPS_DATE) && (m__infos_gps.flg_infos & INFO_GPS_TIME)) {
    unsigned int l__dateAndTime = (unsigned int)strtol(m__infos_gps.t_time, NULL, 10);

#if 0
    char l__buffer[16];
    sprintf(l__buffer, "[%u] => ", m__infos_gps.dateAndTime);
    Serial.print(l__buffer);
    sprintf(l__buffer, "[%u]\n", l__dateAndTime);
    Serial.print(l__buffer);
#endif
  
    if (m__infos_gps.status == 'A' && l__dateAndTime == (m__infos_gps.dateAndTime + 1)) {
      setConnected(true);

      // TODO: Presentation de la date et heure pour une ecriture dans la SDCard 
    }

  /* Nouvelles traces si l'heure courante est inferieure à celle sauvegardee ou 
   * si la date courante est differente de celle sauvegardee
   * => But: Permet de discriminer les nouvelles traces anterieures aux precedentes
   *         mais pas celle postérieures à la meme date et qui seront "attachees"
   *         à ces dernieres (pas genant en soit)
   *         => "New traces" au passage a minuit ;-)
   *
   * => Remarques:
   *    - TBC: Sur le terrain, ne doit jamais arriver sauf si le module GPS fournit
   *           une mauvaise Date/heure par rapport a la precedente
   */
    if (l__dateAndTime < m__infos_gps.dateAndTime
     // Filtrage pour les passages a la minute suivante (ie. HH:57:56 -> HH:58:00)
     || memcmp(m__infos_gps.t_date_save, m__infos_gps.t_date, sizeof(m__infos_gps.t_date_save))
    ) {
			Serial.printf("# New traces\n");
    }

    // Sauvegardes meme si erreur pour le test avec la trame suivante
    m__infos_gps.dateAndTime = l__dateAndTime;

    memcpy(m__infos_gps.t_date_save, m__infos_gps.t_date, sizeof(m__infos_gps.t_date_save));
  }
}

void Nmea::extractInfosGPGGA(char *i__frame)
{
	char *l__pattern = NULL;

	int l__idx = 0;
	l__pattern = strtok(i__frame, ",");
	while (l__pattern != NULL) {
		switch (l__idx) {
		case 9: // Elevation
			strcpy(m__infos_gps.t_elevation, l__pattern);
			m__infos_gps.flg_infos |= INFO_GPS_ELEVATION;

#if USE_TRACE_NMEA_CONTENT
			Serial.printf("\tele [%s] M\n", m__infos_gps.t_elevation);
#endif
			break;

		default:
			break;
		}

		l__pattern = strtok(NULL, ",");
		l__idx++;
	}
}

void Nmea::gestionResponse()
{
    if (m__infos_gps.flg_infos == INFO_GPS_ALL) {
#if USE_TRACE_TLV
      Serial.printf("#%u: TLV [%s]\n", m__idx_tlv++, m__buffer_tlv);
#endif

      if (m__infos_gps.flg_prepare_tlv == true) {
        // Build buffer for "enregistreur de traces" in TLV format
        buildStrForExternal(m__buffer_tlv);

        Serial.printf("#%u: TLV [%s] (%s)\n", m__idx_tlv++, m__buffer_tlv, (m__infos_gps.flg_inh_send_tlv == true ? "No Send" : "Send"));

        /* Recopie dans le buffer d'emission qui est lu caractere par caractere
           au rithme de 2.5 mS
        */
        memset(m__buffer_send, '\0', sizeof(m__buffer_send));
        strncpy(m__buffer_send, m__buffer_tlv, min(sizeof(m__buffer_send) - 1, strlen(m__buffer_tlv)));

        m__infos_gps.flg_prepare_tlv = false;

        m__pos_char_send = 0;
        m__infos_gps.flg_send_tlv = true;

        if (m__infos_gps.flg_inh_send_tlv == false) {
          setActivity();
        }

        // Determination de la Date et Heure a chaque emission de la trame TLV
        g__date_time->buildGpsDateTime(m__infos_gps.t_date, m__infos_gps.t_time,
          m__infos_gps.st_dateAndTimeUNIX_GMT.t_date_time, 
          &m__infos_gps.st_dateAndTime,
          m__infos_gps.st_dateAndTimeUNIX_GMT.t_date_time_lcd);
      
        //Serial.printf("[%s] (epoch [%lu])\n", m__infos_gps.st_dateAndTimeUNIX_GMT.t_date_time, m__infos_gps.st_dateAndTime.epoch);
        //Serial.printf("LCD [%s]\n", m__infos_gps.st_dateAndTimeUNIX_GMT.t_date_time_lcd);

        g__gestion_lcd->Paint_DrawString_EN(6, 8, getDateTimeLcd(), &Font16, BLACK, WHITE);
      }

      m__infos_gps.flg_infos = 0;

      // Gestion Connexion/deconnexion
      setConnected(true);

      g__timers->stop(TIMER_CONNECT);
      g__timers->start(TIMER_CONNECT, DURATION_TIMER_CONNECT, &callback_exec_deconnexion);
    }
}

/* [AA063032.000B1AC94847.0701D1NEA00157.5163F1EG40.19s30.3H6116.55I6150112J204K5117.8L1M]
 * # 1: Type [A] Len [10] Value [063032.000] (*) GPS time HHMMSS.000
   # 2: Type [B] Len [ 1] Value [A]          (*) Valid datas
   # 3: Type [C] Len [ 9] Value [4847.0701]  (*) Lattitude (DDMM.mmmm - minutes decimales)
   # 4: Type [D] Len [ 1] Value [N]          (*) Hemisphere (N ou S)
   # 5: Type [E] Len [10] Value [00157.5163] (*) Longitude (DDMM.mmmm - minutes decimales)
   # 6: Type [F] Len [ 1] Value [E]          (*) Position (E ou W)
   # 7: Type [G] Len [ 4] Value [0.19]           Vitesse en noeuds
   # 8: Type [s] Len [ 3] Value [0.3]            Vitesse en km/h (UUU.D ou ---.-)
   # 9: Type [H] Len [ 6] Value [116.55]         Cap
   #10: Type [I] Len [ 6] Value [150112]     (*) Date DDMMYY
   #11: Type [K] Len [ 5] Value [117.8]      (*) Altitude en metres (UUUU.D ou ----.)
   #12: Type [L] Len [ 1] Value [M]          (*) Unite [M]etre
 */
void Nmea::buildStrForExternal(char *o__text_buffer)
{
	char l__buffer[32];

	*o__text_buffer = '\0';

	sprintf(l__buffer, "A%X", strlen(m__infos_gps.t_time) + strlen(".000"));
	strcat(o__text_buffer, l__buffer);
	strcat(o__text_buffer, m__infos_gps.t_time);
	strcat(o__text_buffer, ".000");

	sprintf(l__buffer, "B1%c", m__infos_gps.status);
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "C%X", strlen(m__infos_gps.t_latitude));
	strcat(o__text_buffer, l__buffer);
	strcat(o__text_buffer, m__infos_gps.t_latitude);

	sprintf(l__buffer, "D1%c", m__infos_gps.north_south == GPS_NORTH ? 'N' : 'S');
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "E%X", strlen(m__infos_gps.t_longitude));
	strcat(o__text_buffer, l__buffer);
	strcat(o__text_buffer, m__infos_gps.t_longitude);

	sprintf(l__buffer, "F1%c", m__infos_gps.east_west == GPS_EAST ? 'E' : 'W');
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "G%X%s", strlen(m__infos_gps.t_speedKnots), m__infos_gps.t_speedKnots);
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "s%X%s", strlen(m__infos_gps.t_speedKmH), m__infos_gps.t_speedKmH);
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "H%X%s", strlen(m__infos_gps.t_cap), m__infos_gps.t_cap);
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "I%X%s", strlen(m__infos_gps.t_date), m__infos_gps.t_date);	
	strcat(o__text_buffer, l__buffer);

	sprintf(l__buffer, "K%X%s", strlen(m__infos_gps.t_elevation), m__infos_gps.t_elevation);	
	strcat(o__text_buffer, l__buffer);

	// TBC: To remove because implicit
	strcat(o__text_buffer, "L1M");

	// Calcul de la checksum
	unsigned char l__cks_calculated = 0x00;
	calculChecksum((char *)o__text_buffer, &l__cks_calculated, NULL);
	sprintf(l__buffer, "*30%02X", l__cks_calculated);		// '*' + 3 bytes de la checksum 0x0HH ;-)
	strcat(o__text_buffer, l__buffer);
}

void Nmea::setConnected(bool i__value)
{
  if (i__value != m__connected) {
    Serial.printf("### %s\n", (i__value == true) ? "Connexion" : "Deconnexion");

    // Ecriture sur la SDCard
    g__sdcard->appendGpsFrame((i__value == true) ? "#Connexion\n" : "#Deconnexion\n");

    // Presentation de l'etat
    g__chenillard = (i__value == true) ? 0xfefe : 0x8080;
  }

  m__connected = i__value;
}

/* Emission caractere par caractere du buffer 'm__buffer_send'
   => Cadencement a 2.5 mS
*/
bool Nmea::sendTlv()
{
  bool l__flg_progress = false;

  if (m__infos_gps.flg_send_tlv == true) {
    //Serial.printf("#%u: Send TLV [%s]\n", m__idx_send++, m__buffer_send);

    if (m__pos_char_send < strlen(m__buffer_send)) {
      g__serial_nmea.write(m__buffer_send[m__pos_char_send++]);

      l__flg_progress = true;
    }
    else {
      g__serial_nmea.write('\n');

      strcat(m__buffer_send, "\n");

      bool l__flg_rtn = g__sdcard->appendGpsFrame(m__buffer_send);

      if (l__flg_rtn) {
        if (g__sdcard->getGpsFrameSize() != 0 && g__sdcard->getInhAppendGpsFrame() == false) {
          char l__buffer[32];
          memset(l__buffer, '\0', sizeof(l__buffer));
          g__sdcard->formatSize(g__sdcard->getGpsFrameSize(), l__buffer);

          Serial.printf("-> Size [%u] bytes [%s]\n", g__sdcard->getGpsFrameSize(), l__buffer);
          g__gestion_lcd->Paint_DrawString_EN(6, 40, l__buffer, &Font16, BLACK, WHITE);
        }
      }

      m__infos_gps.flg_send_tlv = false;
    }
  }

  return l__flg_progress;
}

void Nmea::setActivity()
{
  g__timers->stop(TIMER_NMEA_ACTIVITY);
  g__timers->start(TIMER_NMEA_ACTIVITY, DURATION_TIMER_NMEA_ACTIVITY, &callback_nmea_end_activity);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_YELLOW_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, YELLOW);
}

void Nmea::setError()
{
  g__timers->stop(TIMER_NMEA_ERROR);
  g__timers->start(TIMER_NMEA_ERROR, DURATION_TIMER_NMEA_ERROR, &callback_nmea_end_error);
  g__gestion_lcd->Paint_DrawSymbol(LIGHTS_POSITION_GPS_RED_X, LIGHTS_POSITION_Y, LIGHT_FULL_IDX, &Font24Symbols, BLACK, RED);
}
