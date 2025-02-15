// $Id: GpsPilot.h,v 1.11 2025/02/12 14:10:20 administrateur Exp $

#ifndef __GPS_PILOT__
#define __GPS_PILOT__

// 'GpsPilot.txt' a lire depuis la SDCard dans le repertoire '/PILOT'
constexpr char *GPSPILOT_FILE_NAME = "/PILOT/GpsPilot.txt";

/* Definition des trames compatibles avec la version Arduino UNO du 'GpsRecorder'
   => TODO; To improve...
*/
constexpr char *TEXT_GPS_PILOT_UNO_HEADER = "\n\n\n#UNO: GPS Recorder - Version 1.3.3";
constexpr char *TEXT_GPS_PILOT_UNO_BEGIN  = "1>Begin";
constexpr char *TEXT_GPS_PILOT_UNO_END    = "1>End";
constexpr char *TEXT_GPS_PILOT_ERROR      = "Error";    // Provoquera une erreur a la reception ;-)

typedef enum {
  PROGRESS_SEND_IDLE = 0,
  PROGRESS_SEND_HEADER_IN_PROGRESS,   // Emission de 'TEXT_GPS_PILOT_UNO_HEADER'
  PROGRESS_SEND_HEADER_DONE,
  PROGRESS_SEND_BEGIN_IN_PROGRESS,    // Emission de 'TEXT_GPS_PILOT_UNO_BEGIN'
  PROGRESS_SEND_BEGIN_DONE,
  PROGRESS_SEND_DATAS_IN_PROGRESS,    // Emission ligne par ligne du fichier '/PILOT/GpsPilot.txt' de la SDCard
  PROGRESS_SEND_DATAS_LAST_LINE,
  PROGRESS_SEND_DATAS_DONE,
  PROGRESS_SEND_END_IN_PROGRESS,      // Emission de 'TEXT_GPS_PILOT_UNO_END'
  PROGRESS_SEND_END_DONE
} ENUM_PROGRESS_SEND;

typedef struct {
  size_t                  buffer_num;
  size_t                  size_file;
  size_t                  size_read;
  size_t                  idx_line;

  const char              *file_name;
  String                  name_trace;
  String                  distance;
  String                  cumul_ele_pos;
  String                  cumul_ele_neg;

  uint32_t                cks_value;
  size_t                  cks_size;
} ST_PROPERTIES;

// Attributs pour le calcul de la checksum incrementale
typedef struct {
  uint32_t            crc_tmp;
  size_t              size_cumul;
  uint32_t            checksum_calc;
  size_t              size_datas_calc;
} ST_CALC_CHECKSUM;

class GpsPilot {
  private:
    ENUM_PROGRESS_SEND      m__progress;

    size_t                  m__buffer_idx;
    String                  m__buffer_to_send;

    ST_PROPERTIES           m__properties;
    ST_CALC_CHECKSUM        m__calc_cks;

    void resetProperties();

    void prepareBufferToSend(const char *i__text);

    void getTokenStrValue(const char *i__text, String &o__propertie);
    void getTokenNumValue(const char *i__text, String &o__propertie);
    void getTokenCksValue(const char *i__text);

    void calculChecksumIncClearValues();
    void calculChecksumInc(const uint8_t *i__datas, size_t i__size, bool i__flg_end = false);

  public:
    GpsPilot();
    ~GpsPilot();

    bool isInProgress() const { return (m__progress != PROGRESS_SEND_IDLE); };

    bool readFile(String &o__line, bool *o__end_of_file);

    bool isBufferToSend() const { return (m__progress != PROGRESS_SEND_IDLE && m__buffer_to_send.length() != 0); };
    void send();
    void sendBuffer();

    const String &getTraceName() const { return m__properties.name_trace; };
    const String &getTraceDist() const { return m__properties.distance; };
};

extern GpsPilot   *g__gps_pilot;
#endif
