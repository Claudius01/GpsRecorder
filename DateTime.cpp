// $Id: DateTime.cpp,v 1.7 2025/02/11 13:52:38 administrateur Exp $

#include <Arduino.h>

#include "Misc.h"
#include "DateTime.h"

static byte                 g__monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static ST_FOR_SOMMER_TIME_CHANGE   g__st_for_sommer_time_change = {
  false,
  {
    0, 0, 0,
    0, 0, 0,
    0
  },
  {
    0, 0, 0,
    0, 0, 0,
    0
  },
  { 
    { JEUDI,    "Jeudi" },
    { VENDREDI, "Vendredi" },
    { SAMEDI,   "Samedi" },
    { DIMANCHE, "Dimanche" },
    { LUNDI,    "Lundi" },
    { MARDI,    "Mardi" },
    { MERCREDI, "Mercredi" }
  }
};

DateTime::DateTime() : epoch_start(0L),epoch(0), epoch_diff(0L), duration_deconnexion(0L)
{
	Serial.println("DateTime::DateTime()");
}

DateTime::~DateTime()
{
	Serial.println("DateTime::~DateTime()");
}

/* Convert broken time to calendar time (seconds since 1970)
 *
 * Warning: Date limite en 2038 ;-)
*/
long DateTime::my_mktime(ST_TM *timeptr)
{
  int  year  = timeptr->tm_year + 1900;
  int  month = timeptr->tm_mon;
  int  i;
  long seconds;

  // Seconds from 1970 till 1 jan 00:00:00 this year
  seconds = (long)(year - 1970) * 60L * 60 * 24 * 365;  // seconds is a long

  // Add extra days for leap years
  for (i = 1970; i < year; i++) {
    if (LEAP_YEAR(i)) {
      seconds += 60L * 60 * 24;        // Add one day for each bisecstil year
    }
  }

    // Add days for this year
  for (i = 0; i < month; i++) {
    if (i == 1 && LEAP_YEAR(year)) { 
      seconds += 60L * 60 * 24 * 29;  // seconds is a long
    }
    else {
      seconds += (long)g__monthDays[i] * 60L * 60 * 24;  // seconds is a long
    }
  }

  seconds += (long)(timeptr->tm_mday - 1) * 60L * 60 * 24;  // seconds is a long
  seconds += (long)timeptr->tm_hour * 60L * 60;
  seconds += (long)timeptr->tm_min * 60L;
  seconds += (long)timeptr->tm_sec;

  return seconds;
}

/* Calcul du temps en secondes depuis le 01/01/1970 - 00:00:00
*/
long DateTime::calculatedEpochTime(ST_DATE_AND_TIME *i__st_date_and_time)
{
    ST_TM l__st_tm;
    memset(&l__st_tm, '\0', sizeof(ST_TM));

    l__st_tm.tm_mday = i__st_date_and_time->day;
    l__st_tm.tm_mon  = i__st_date_and_time->month - 1;
    l__st_tm.tm_year = i__st_date_and_time->year + (2000 - 1900);
    l__st_tm.tm_hour = i__st_date_and_time->hours;
    l__st_tm.tm_min  = i__st_date_and_time->minutes;
    l__st_tm.tm_sec  = i__st_date_and_time->seconds;

    return my_mktime(&l__st_tm);
}

// TODO: Add test of all fields
long DateTime::buildGpsDateTime(const char i__date[], const char i__time[], char *o_date_time, ST_DATE_AND_TIME *o__dateAndTime, char *o__text_for_lcd)
{
	ST_DATE_AND_TIME l__dateAndTime;
	memset(&l__dateAndTime, '\0', sizeof(ST_DATE_AND_TIME));

	char l__t_wrk[3];
	memset(l__t_wrk, '\0', sizeof(l__t_wrk));

	// Set the fields Year, Month, Day, Hours, Minutes and Seconds
	// Year as 20XX
	strncpy(l__t_wrk, &i__date[4], 2);
	l__dateAndTime.year = (int)strtol(l__t_wrk, NULL, 10);

	// Month
	strncpy(l__t_wrk, &i__date[2], 2);
	l__dateAndTime.month = (char)strtol(l__t_wrk, NULL, 10);

	// Day
	strncpy(l__t_wrk, &i__date[0], 2);
	l__dateAndTime.day = (char)strtol(l__t_wrk, NULL, 10);

	// Hour
	strncpy(l__t_wrk, &i__time[0], 2);
	l__dateAndTime.hours = (char)strtol(l__t_wrk, NULL, 10);

	// Minutes
	strncpy(l__t_wrk, &i__time[2], 2);
	l__dateAndTime.minutes = (char)strtol(l__t_wrk, NULL, 10);

	// Seconds
	strncpy(l__t_wrk, &i__time[4], 2);
	l__dateAndTime.seconds = (char)strtol(l__t_wrk, NULL, 10);
	// End: Set the fields Year, Month, Day, Hours, Minutes and Seconds

  // Get the current epoch and save into datas
  long l__epoch = calculatedEpochTime(&l__dateAndTime);
  l__dateAndTime.epoch = l__epoch;

  // Calculation of day of the week
  l__dateAndTime.day_of_week = (char)((l__epoch / 86400L) % 7L);

  // Détermination changement d'heure été/hiver si pas déjà fait
  if (g__st_for_sommer_time_change.flg_available == false) {
    setSommerWinterTimeChange(&l__dateAndTime);
  }

  // Application heure été/hiver
  calcSommerTimeChange(&l__dateAndTime);

  memcpy(o__dateAndTime, &l__dateAndTime, sizeof(ST_DATE_AND_TIME));

  const char *l__sommer_winter = "Unknown";
  if (o__dateAndTime->sommer_winter == SOMMER) {
    l__sommer_winter = "Heure d'été";
  }
  else if (o__dateAndTime->sommer_winter == WINTER) {
    l__sommer_winter = "Heure d'hiver";
  }

  // Présentation de la date, heure, jour de la semaine avec application de l'heure d'été/hiver
  ST_DATE_AND_TIME l__dateAndTime_presentation;
  memcpy(&l__dateAndTime_presentation, o__dateAndTime, sizeof(ST_DATE_AND_TIME));
  applySommerWinterHour(&l__dateAndTime_presentation);

  sprintf(o_date_time, "%04d/%02d/%02d %02d:%02d:%02d (#%d/#%d %s) (%s)",
    2000 + l__dateAndTime_presentation.year, l__dateAndTime_presentation.month, l__dateAndTime_presentation.day,
    l__dateAndTime_presentation.hours, l__dateAndTime_presentation.minutes, l__dateAndTime_presentation.seconds,
    l__dateAndTime_presentation.nbr_days_before + 1,
    l__dateAndTime_presentation.nbr_days_before + 1 + l__dateAndTime_presentation.nbr_days_after,    
    g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime_presentation.day_of_week].name,
    l__sommer_winter);

  /* Presentation pour LCD
                  1          2         3
          123456789012345678901234567890
     ie. "Ven. 31/01/25 18:35" 
  */
  if (o__text_for_lcd != NULL) {
    char l__day_of_week[3 + 1];
    memset(l__day_of_week, '\0', sizeof(l__day_of_week));

    strncpy(l__day_of_week, g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime_presentation.day_of_week].name, 3);

    sprintf(o__text_for_lcd, "%s. %02d/%02d/%04d %02d:%02d",
      l__day_of_week,
      l__dateAndTime_presentation.day, l__dateAndTime_presentation.month, (2000 + l__dateAndTime_presentation.year),
      l__dateAndTime_presentation.hours, l__dateAndTime_presentation.minutes);
  }
  // Presentation pour LCD

	return l__epoch;
}

bool DateTime::setSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime)
{
#if 0   // TBC: Interference avec les traces d'emission de 'GpsPilot.txt'
  char l__buffer[80];
#endif

  ST_DATE_AND_TIME l__dateAndTime;
  bool l__flg_available_sommer = false;
  bool l__flg_available_winter = false;

  // Détermination du passage à l'heure d'été (dernier Dimanche de Mars)
  // Set at YEAR/03/01 - 00:00:00
  memset(&l__dateAndTime, '\0', sizeof(ST_DATE_AND_TIME));
  l__dateAndTime.year = i__dateAndTime->year;
  l__dateAndTime.month = 3;
  l__dateAndTime.day = 1;

  long l__epoch = calculatedEpochTime(&l__dateAndTime);

  // Calculation of day of the week
  l__dateAndTime.day_of_week = (char)((l__epoch / 86400L) % 7L);

#if 0   // TBC: Interference avec les traces d'emission de 'GpsPilot.txt'
  sprintf(l__buffer, "Epoch UNIX GMT [%ld] (%04d/%02d/%02d %02d:%02d:%02d [%s])\n",
    l__epoch,
    2000 + l__dateAndTime.year, l__dateAndTime.month, l__dateAndTime.day,
    l__dateAndTime.hours, l__dateAndTime.minutes, l__dateAndTime.seconds,
    g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime.day_of_week].name);

  Serial.print(l__buffer);
#endif

  // Parcours jusqu'à trouver le dernier Dimanche du mois
  byte n = 0;
  byte l__nbr_days = g__monthDays[(int)l__dateAndTime.month - 1];
  for (n = l__dateAndTime.day; n <= l__nbr_days; n++) {
    byte l__day_of_week = (byte)((l__epoch / 86400L) % 7L);

    if (l__day_of_week == DIMANCHE) {
      byte l__nbr_days_after = (l__nbr_days - n) / 7;

      if (l__nbr_days_after == 0) {
        g__st_for_sommer_time_change.st_date_sommer.day = n;
        g__st_for_sommer_time_change.st_date_sommer.month = l__dateAndTime.month;
        g__st_for_sommer_time_change.st_date_sommer.year = l__dateAndTime.year;
        g__st_for_sommer_time_change.st_date_sommer.day_of_week = l__day_of_week;
        g__st_for_sommer_time_change.st_date_sommer.epoch = l__epoch + 3600L;     // Change at 01:00:00

        l__flg_available_sommer = true;

#if 0   // TBC: Interference avec les traces d'emission de 'GpsPilot.txt'
        sprintf(l__buffer, "\tDernier %s du mois [%04d/%02d/%02d 01:00:00] (%ld)\n",
          g__st_for_sommer_time_change.st_days_in_week[(int)g__st_for_sommer_time_change.st_date_sommer.day_of_week].name,
          2000 + g__st_for_sommer_time_change.st_date_sommer.year,
          g__st_for_sommer_time_change.st_date_sommer.month,
          g__st_for_sommer_time_change.st_date_sommer.day,
          g__st_for_sommer_time_change.st_date_sommer.epoch);

        Serial.print(l__buffer);
#endif

        break;
      }
    }

    l__epoch += 86400L;
  }
  // Fin: Détermination du passage à l'heure d'été (dernier Dimanche de Mars)

  // Détermination du passage à l'heure d'hiver (dernier Dimanche d'octobre)
  // Set at YEAR/10/01 - 00:00:00
  memset(&l__dateAndTime, '\0', sizeof(ST_DATE_AND_TIME));
  l__dateAndTime.year = i__dateAndTime->year;
  l__dateAndTime.month = 10;
  l__dateAndTime.day = 1;

  l__epoch = calculatedEpochTime(&l__dateAndTime);

  // Calculation of day of the week
  l__dateAndTime.day_of_week = (char)((l__epoch / 86400L) % 7L);

#if 0   // TBC: Interference avec les traces d'emission de 'GpsPilot.txt'
  sprintf(l__buffer, "Epoch UNIX GMT [%ld] (%04d/%02d/%02d %02d:%02d:%02d [%s])\n",
    l__epoch,
    2000 + l__dateAndTime.year, l__dateAndTime.month, l__dateAndTime.day,
    l__dateAndTime.hours, l__dateAndTime.minutes, l__dateAndTime.seconds,
    g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime.day_of_week].name);

  Serial.print(l__buffer);
#endif

  // Parcours jusqu'à trouver le dernier Dimanche du mois
  n = 0;
  l__nbr_days = g__monthDays[(int)l__dateAndTime.month - 1];
  for (n = l__dateAndTime.day; n <= l__nbr_days; n++) {
    byte l__day_of_week = (byte)((l__epoch / 86400L) % 7L);

    if (l__day_of_week == DIMANCHE) {
      byte l__nbr_days_after = (l__nbr_days - n) / 7;

      if (l__nbr_days_after == 0) {
        g__st_for_sommer_time_change.st_date_winter.day = n;
        g__st_for_sommer_time_change.st_date_winter.month = l__dateAndTime.month;
        g__st_for_sommer_time_change.st_date_winter.year = l__dateAndTime.year;
        g__st_for_sommer_time_change.st_date_winter.day_of_week = l__day_of_week;
        g__st_for_sommer_time_change.st_date_winter.epoch = l__epoch + 3600L;     // Change at 01:00:00

        l__flg_available_winter = true;

#if 0   // TBC: Interference avec les traces d'emission de 'GpsPilot.txt'
        sprintf(l__buffer, "\tDernier %s du mois [%04d/%02d/%02d 01:00:00] (%ld)\n",
          g__st_for_sommer_time_change.st_days_in_week[(int)g__st_for_sommer_time_change.st_date_winter.day_of_week].name,
          2000 + g__st_for_sommer_time_change.st_date_winter.year,
          g__st_for_sommer_time_change.st_date_winter.month,
          g__st_for_sommer_time_change.st_date_winter.day,
          g__st_for_sommer_time_change.st_date_winter.epoch);

        Serial.print(l__buffer);
#endif

        break;
      }
    }

    l__epoch += 86400L;
  }
  // Fin: Détermination du passage à l'heure d'hiver (dernier Dimanche d'octobre)

  if (l__flg_available_sommer == true && l__flg_available_winter == true) {
    g__st_for_sommer_time_change.flg_available = true;
  }
  else {
    g__st_for_sommer_time_change.flg_available = false;
  }

  return g__st_for_sommer_time_change.flg_available;
}

void DateTime::calcSommerTimeChange(ST_DATE_AND_TIME *io__dateAndTime)
{
#if 0
    char l__buffer[80];
#endif

    io__dateAndTime->nbr_days_before = (io__dateAndTime->day - 1) / 7;            // Nombre du même jour avant dans le mois

    byte l__nbr_days = g__monthDays[(int)io__dateAndTime->month - 1];
    if (io__dateAndTime->month == 2 && LEAP_YEAR(io__dateAndTime->year)) {
      l__nbr_days = 29;   // 29 jours en février les années bissextiles
    }
    io__dateAndTime->nbr_days_after = (l__nbr_days - io__dateAndTime->day) / 7;   // Nombre du même jour après dans le mois

    /* Détermination heure été/hiver:
     *   - Du dernier Dimance d'Octobre au dernier Dimanche de Mars: Heure d'hiver
     *   - Du dernier Dimanche de Mars au dernier Dimance d'Octobre: Heure d'été
     */
    io__dateAndTime->sommer_winter = getSommerWinterTimeChange(io__dateAndTime);

#if 0
    sprintf(l__buffer, "calcSommerTimeChange(): %s (%d/%d) Sommer/Winter [%d]\n",
      g__st_for_sommer_time_change.st_days_in_week[(int)io__dateAndTime->day_of_week].name,
      io__dateAndTime->nbr_days_before + 1,
      io__dateAndTime->nbr_days_before + 1 + io__dateAndTime->nbr_days_after,
      io__dateAndTime->sommer_winter);

    Serial.print(l__buffer);
#endif
}

ENUM_SOMMER_WINTER DateTime::getSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime)
{
  ENUM_SOMMER_WINTER l__sommer_winter = NO_SOMMER_WINTER;

#if 0   // Version with month/day 00:00:00 test
  if (g__st_for_sommer_time_change.flg_available == true) {
    if (i__dateAndTime->month > 3 && i__dateAndTime->month < 10) {
      // Heure d'été pour les mois d'Avril, Mai, Juin, Juillet, Aout et Septembre
      l__sommer_winter = SOMMER;
    }
    else if (i__dateAndTime->month < 3 || i__dateAndTime->month > 10) {
      // Heure d'hiver pour les mois de Janvier, Février, Novembre et Décembre
      l__sommer_winter = WINTER;
    }
    else if (i__dateAndTime->month == 3) {
      /* Heure d'été pour le mois de Mars avant la date de basculement à 00:00:00 GMT
       *  => Remarque: Le basculement devrait s'effectuer à 01:00:00
       */
      l__sommer_winter = (i__dateAndTime->day < g__st_for_sommer_time_change.st_date_sommer.day) ? WINTER : SOMMER;
    }
    else if (i__dateAndTime->month == 10) {
      /* Heure d'hiver pour le mois d'Octobre après la date de basculement à 00:00:00 GMT
       *  => Remarque: Le basculement devrait s'effectuer à 01:00:00
       */
      l__sommer_winter = (i__dateAndTime->day < g__st_for_sommer_time_change.st_date_winter.day) ? SOMMER : WINTER;      
    }
  }
#else   // Version with epoch test
  if (i__dateAndTime->epoch <  (g__st_for_sommer_time_change.st_date_sommer.epoch)
   || i__dateAndTime->epoch >= (g__st_for_sommer_time_change.st_date_winter.epoch)) {
    l__sommer_winter = WINTER;
  }
  else if (i__dateAndTime->epoch >= (g__st_for_sommer_time_change.st_date_sommer.epoch)
        && i__dateAndTime->epoch <  (g__st_for_sommer_time_change.st_date_winter.epoch)) {
    l__sommer_winter = SOMMER;
  }
#endif

  return l__sommer_winter;
}

void DateTime::applySommerWinterHour(ST_DATE_AND_TIME *io__dateAndTime_presentation)
{
  byte l__nbr_hours = 0;
  if (io__dateAndTime_presentation->sommer_winter == WINTER) {
    l__nbr_hours += 1;

    // Update for statistics
    //g__stats->setOffsetSeconds(3600);
  }
  else if (io__dateAndTime_presentation->sommer_winter == SOMMER) {
    l__nbr_hours += 2;

    // Update for statistics
    //g__stats->setOffsetSeconds(2 * 3600);
  }

  io__dateAndTime_presentation->hours += l__nbr_hours;
  if (io__dateAndTime_presentation->hours > 23) {
    //  Passage à minuit => +1 sur les jours dans le mois et dans la semaine
    io__dateAndTime_presentation->hours %= 24;
    io__dateAndTime_presentation->day += 1;
    io__dateAndTime_presentation->day_of_week += 1;
    io__dateAndTime_presentation->day_of_week %= 7;

    // Test si changement de mois, voire d'année ;-)
    byte l__nbr_days = g__monthDays[(int)io__dateAndTime_presentation->month];

    // Années bissextiles
    if (io__dateAndTime_presentation->month == 2 && LEAP_YEAR(io__dateAndTime_presentation->year)) {
      l__nbr_days = 29;
    }

    if (io__dateAndTime_presentation->day > l__nbr_days) {
      io__dateAndTime_presentation->day = 1;
      io__dateAndTime_presentation->month += 1;

      if (io__dateAndTime_presentation->month > 12) {
        io__dateAndTime_presentation->month = 1;
        io__dateAndTime_presentation->year += 1;
      }
    }
  }
}

void DateTime::formatEpochDiff(char *o__buffer) const
{
  formatDuration(o__buffer, epoch_diff);
}

void DateTime::formatDurationDeconnexion(char *o__buffer) const
{
  formatDuration(o__buffer, duration_deconnexion);
}

void DateTime::formatDuration(char *o__buffer, long i__value) const
{
  if (i__value < 3600L) {
    // Formatage comme "MM'SS" si 'i__value' < 1 Heure
    sprintf(o__buffer, "%2lu'%02lu", (i__value / 60L), (i__value % 60L));
  }
  else if (i__value < (100L * 3600L)) {
    // Formatage comme "HH:MM" si 'i__value' < 100 Heures
    sprintf(o__buffer, "%02lu:%02lu", (i__value / 3600L), (i__value % 3600L) / 60L);
  }
  else {
    // Formatage comme ">100H" si 'i__value' >= 100 Heures
    sprintf(o__buffer, ">100H");
  }
}
