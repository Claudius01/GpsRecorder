// $Id: SDCard.h,v 1.4 2025/02/04 15:03:36 administrateur Exp $

#ifndef __SDCARD__
#define __SDCARD__

/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32-S3-GEEK
 *    D2       38
 *    D3       34(SS)
 *    CMD      35(MOSI)
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      36(SCK)
 *    VSS      GND
 *    D0       37(MISO)
 *    D1       33
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Define the pins of the first SPI bus
#define SPI1_SCK  36
#define SPI1_MISO 37
#define SPI1_MOSI 35
#define SPI1_SS   34

// @KiCad: MISO   19
// @KiCad: SCK    18
// @KiCad: MOSI   23

#define NAME_OF_FILE_GPS_FRAMES       "/FRAME/GpsFrames.txt"

class SDCard {
  private:
    bool        flg_init;
    int         nbr_of_init_retry;
    uint8_t     cardType;
    uint64_t    cardSize;

    bool        flg_sdcard_in_use;
    bool        flg_inh_append_gps_frame;

    size_t      m__gps_frame_size;

    bool preparing();

  public:
    SDCard();
    ~SDCard();

    bool init();
    void init(int i__nbr_retry);
    void end();

    void startActivity();
    void stopActivity(boolean i__flg_no_error = true);

    void callback_sdcard_retry_init_more();

    bool isInit()  { return flg_init; };
    bool isInUse() { return flg_sdcard_in_use; };

    // Methods for '/FRAME/GpsFrames.txt' gestion
    void setInhAppendGpsFrame(bool i__flg) { flg_inh_append_gps_frame = i__flg; };
    bool getInhAppendGpsFrame() const { return flg_inh_append_gps_frame; };

    bool appendGpsFrame(const char *i__frame, boolean i__flg_force_append = false);
    size_t getGpsFrameSize() const { return m__gps_frame_size; };
    void formatSize(size_t i__value, char *o__buffer);

    // Methods for tests
    bool printInfos();   
    bool listDir(const char *i__dir);
    bool exists(const char *i__path);
    bool readFile(const char *i__file);
    bool getFileLine(const char *i__file, String &o__line, bool i__flg_close = false);
    bool appendFile(const char *i__file, const char *i__line);
    bool renameFile(const char *i__path_from, const char *i__path_to);
    bool deleteFile(const char *i__path);
    // End: Methods for tests
};

extern void callback_end_sdcard_acces();
extern void callback_end_sdcard_error();
extern void callback_sdcard_retry_init();
extern void callback_sdcard_init_error();

extern SDCard     *g__sdcard;
extern bool       g__flg_inh_sdcard_ope;
#endif
