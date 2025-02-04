// $Id: Nmea.h,v 1.6 2025/02/04 12:55:08 administrateur Exp $

#ifndef __NMEA__
#define __NMEA__

#define min(a, b)    (a < b) ? a : b
#define max(a, b)    (a > b) ? a : b

// Pins de l'ESP32-S3-GEEK
#define NMEA_TXD                13      // GPIO13
#define NMEA_RXD                14      // GPIO14

#define NUMBER_OF_CHAR_REC      128
#define NUMBER_OF_CHAR_TLV      128
#define NUMBER_OF_CHAR_SEND     128

#define INFO_GPS_STATUS       (1 <<  9)
#define INFO_GPS_TIME         (1 <<  8)
#define INFO_GPS_DATE         (1 <<  7)
#define INFO_GPS_LAT			  	(1 <<  6)
#define INFO_GPS_LAT_DIR	  	(1 <<  5)
#define INFO_GPS_LON		  		(1 <<  4)
#define INFO_GPS_LON_DIR		  (1 <<  3)
#define INFO_GPS_SPEED		  	(1 <<  2)
#define INFO_GPS_CAP			  	(1 <<  1)
#define INFO_GPS_ELEVATION    (1 <<  0)

#define INFO_GPS_ALL	(INFO_GPS_STATUS | INFO_GPS_TIME | INFO_GPS_DATE | INFO_GPS_LAT | INFO_GPS_LAT_DIR | INFO_GPS_LON | INFO_GPS_LON_DIR | INFO_GPS_SPEED | INFO_GPS_CAP | INFO_GPS_ELEVATION)

typedef enum {
	GPS_UNKNOWN = 0,
	GPS_NORTH,
	GPS_EAST,
	GPS_SOUTH,
	GPS_WEST
} ENUM_GPS_DIRECTION;

typedef struct {
  bool                flg_inh_send_tlv;
	bool                flg_prepare_tlv;
  bool                flg_send_tlv;

	unsigned int        flg_infos;
	char                status;                     // 'A' pour une trame 'GPRMC' valide
	char                t_time[6 + 1];              // HHMMSS + '\0'
	char                t_date[6 + 1];              // DDMMYY + '\0'
	char                t_date_save[6 + 1]; 				// Pour la détection "New traces"
	unsigned int        dateAndTime;
	char                t_latitude[4 + 1 + 4 + 1];  // ie. 4847.0720 + '\0'
	ENUM_GPS_DIRECTION  north_south;
	char                t_longitude[5 + 1 + 4 + 1];	// ie. 00157.5211 + '\0'
	ENUM_GPS_DIRECTION  east_west;
	char                t_speedKnots[8];
	char                t_speedKmH[16];
	char                t_cap[8];
	char                t_elevation[8];             // uuuu.d + '\0' ou -uuu.d + '\0'

  ST_DATE_AND_TIME    st_dateAndTime;
  ST_UNIX_TIME        st_dateAndTimeUNIX_GMT;
} ST_INFOS_GPS;

class Nmea {
  private:
    uint16_t          m__idx_frames;
    size_t            m__size_buff_rec;
    char              m__buffer_rec[NUMBER_OF_CHAR_REC];

    ST_INFOS_GPS      m__infos_gps;

    uint16_t          m__idx_tlv;
    char              m__buffer_tlv[NUMBER_OF_CHAR_TLV];

    uint16_t          m__idx_send;
    size_t            m__pos_char_send;
    char              m__buffer_send[NUMBER_OF_CHAR_SEND];

    bool              m__connected;

  public:
    Nmea();
    ~Nmea();

    void              readFrame();
    bool              calculChecksum(char *i__frame, unsigned char *o__cks_calculated, unsigned char *o__cks_expected);
    void              extractInfosGPRMC(char *i__frame);
    void              extractInfosGPGGA(char *i__frame);
    void              convertSpeedKnotsToKmH();

    bool              getInhSendTlv() const { return m__infos_gps.flg_inh_send_tlv; };
    void              setInhSendTlv(bool i__value) { m__infos_gps.flg_inh_send_tlv = i__value; };
    bool              sendTlv();

    void              gestionResponse();
    void              buildStrForExternal(char *o__text_buffer);

    bool              getConnected() const { return m__connected; };
    void              setConnected(bool i__value);
    void              setActivity();
    void              setError();

    const char        *getDateTimeLcd() const { return m__infos_gps.st_dateAndTimeUNIX_GMT.t_date_time_lcd; };
};

extern uint16_t     g__chenillard;
extern DateTime     *g__date_time;
extern Nmea         *g__nmea;
extern GestionLCD   *g__gestion_lcd;
#endif
