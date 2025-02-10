// $Id: DateTime.h,v 1.5 2025/02/10 16:35:10 administrateur Exp $

#ifndef __DATE_TIME__
#define __DATE_TIME__

#include <sstream>

// Années bissextiles
#define LEAP_YEAR(year) ((year % 4) == 0)

typedef enum {
	GMT,
	LOCALTIME
} DEF_GMT_LOCAL;

typedef enum {
  NO_SOMMER_WINTER = 0,
  SOMMER,
  WINTER
} ENUM_SOMMER_WINTER;

typedef struct {
  int tm_sec;    /* Seconds. [0-60] (1 leap second) */
  int tm_min;    /* Minutes. [0-59] */
  int tm_hour;   /* Hours. [0-23] */
  int tm_mday;   /* Day.   [1-31] */
  int tm_mon;    /* Month. [0-11] */
  int tm_year;   /* Year - 1900.  */
} ST_TM;

typedef struct {
  char                  seconds;              // Seconde [0..59]
  char                  minutes;
  char                  hours;
  char                  day;
  char                  month;
  char                  year;                 // Year same 20XX
  char                  day_of_week;
  byte                  nbr_days_before;      // Nombre du même jour avant et après dans le mois
  byte                  nbr_days_after;       // (nbr_days_before + 1 + nbr_days_after) = Nbr total du même jour
  ENUM_SOMMER_WINTER    sommer_winter;
  long                  epoch;
} ST_DATE_AND_TIME;

typedef struct {
  time_t                epoch;
  char                  t_date_time[132];       // Date/Time formated (ie. [2025/01/31 18:08:14 (#5/#5 Vendredi) (Heure d'hiver)])
  char                  t_date_time_lcd[32];    // Date/Time formated (ie. [Ven. 31/01/25 18:08])
} ST_UNIX_TIME;

/* Définitions pour la détermination du changement d'heure été/hiver avec
   le numéro du jour dans la semaine (dernier dimance de mars et octobre)
*/
typedef enum {
  JEUDI = 0,
  VENDREDI,
  SAMEDI,
  DIMANCHE,
  LUNDI,
  MARDI,
  MERCREDI
} ENUM_DAYS_OF_WEEK;

typedef struct {
  ENUM_DAYS_OF_WEEK   num_day;              // 0: Jeudi, 1: Vendredi, ..., 6: Mercredi
  const char          *name;                // car le 01/01/1970 est un Jeudi ;-)
} ST_DAYS_IN_WEEK;

typedef struct {
  bool                flg_available;        // Dates de basculement disponibles (true/false)
  ST_DATE_AND_TIME    st_date_sommer;       // Date de basculement heure d'été
  ST_DATE_AND_TIME    st_date_winter;       // Date de basculement heure d'hiver
  ST_DAYS_IN_WEEK     st_days_in_week[7];   // => Si nbr_days_after = 0, ce jour est le dernier du mois ;-)
} ST_FOR_SOMMER_TIME_CHANGE;
// Fin: Définitions pour la détermination du changement d'heure été/hiver

class DateTime {
	private:
    long epoch_start;
    long epoch;
    long epoch_diff;

		long my_mktime(ST_TM *timeptr);
		long calculatedEpochTime(ST_DATE_AND_TIME *i__st_date_and_time);
		bool setSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime);
		void calcSommerTimeChange(ST_DATE_AND_TIME *io__dateAndTime);
		ENUM_SOMMER_WINTER getSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime);
		void applySommerWinterHour(ST_DATE_AND_TIME *io__dateAndTime_presentation);

	public:
		DateTime();
 		~DateTime();

		long buildGpsDateTime(const char i__date[], const char i__time[], char *o_date_time, ST_DATE_AND_TIME *o__dateAndTime, char *o__text_for_lcd = NULL);

    long getEpochStart() const { return epoch_start; };
    void setEpochStart(long i__value) { epoch_start = i__value; };
    long getEpoch() const { return epoch; };
    void setEpoch(long i__value) { epoch = i__value; epoch_diff = (epoch - epoch_start); };
    long getEpochDiff() const { return epoch_diff; };

    void formatEpochDiff(char *o__buffer) const;
};

extern DateTime   *g__date_time;
#endif
