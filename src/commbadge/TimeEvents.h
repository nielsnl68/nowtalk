//  TimeAlarms.h - Arduino Time alarms header for use with Time library

#ifndef TimeEvents_h
#define TimeEvents_h

#include <Arduino.h>
#include <soc/rtc.h>
extern "C" {
#include <esp_clk.h>
}

#define USE_SPECIALIST_METHODS  // define this for testing
typedef enum {
    dowInvalid, dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday
} timeDayOfWeek_t;
#define SECS_PER_MIN  ((uint32_t)(60UL))
#define SECS_PER_HOUR ((uint32_t)(3600UL))
#define SECS_PER_DAY  ((uint32_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((uint32_t)(7UL))
#define SECS_PER_WEEK ((uint32_t)(SECS_PER_DAY * DAYS_PER_WEEK))

#define SECS_PER_YEAR ((uint32_t)(SECS_PER_DAY * 365UL)) // TODO: ought to handle leap years

#define secs_per_year(y) ((uint32_t)(SECS_PER_DAY * year_lengths[isleap(y)]))
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define dayOfWeek(_time_)  ((( _time_ / SECS_PER_DAY + 4)  % DAYS_PER_WEEK)+1) // 1 = Sunday
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  // this is number of days since Jan 1 1970
#define elapsedSecsToday(_time_)  (_time_ % SECS_PER_DAY)   // the number of seconds since last midnight 
// The following macros are used in calculating alarms and assume the clock is set to a date later than Jan 1 1971
// Always set the correct time before settting alarms
#define previousMidnight(_time_) (( _time_ / SECS_PER_DAY) * SECS_PER_DAY)  // time at the start of the given day
#define nextMidnight(_time_) ( previousMidnight(_time_)  + SECS_PER_DAY )   // time at the end of the given day 
#define elapsedSecsThisWeek(_time_)  (elapsedSecsToday(_time_) +  ((dayOfWeek(_time_)-1) * SECS_PER_DAY) )   // note that week starts on day 1
#define previousSunday(_time_)  (_time_ - elapsedSecsThisWeek(_time_))      // time at the start of the week for the given time
#define nextSunday(_time_) ( previousSunday(_time_)+SECS_PER_WEEK)          // time at the end of the week for the given time

#define dtNBR_ALARMS 6   // max is 255
#define dtINVALID_ALARM_ID 255
#define dtINVALID_TIME     (uint32_t)(-1)

// new time based alarms should be added just before dtLastAlarmType
typedef enum {
    dtNotAllocated,
    dtTimer,
    dtExplicitAlarm,
    dtDailyAlarm,
    dtWeeklyAlarm,
    dtLastAlarmType
} dtAlarmPeriod_t; // in future: dtBiweekly, dtMonthly, dtAnnual

typedef struct {
    uint8_t alarmType : 4;  // enumeration of daily/weekly (in future:
                                 // biweekly/semimonthly/monthly/annual)
                                 // note that the current API only supports daily
                                 // or weekly alarm periods
    uint8_t isEnabled : 1;  // the timer is only actioned if isEnabled is true
    uint8_t isOneShot : 1;  // the timer will be de-allocated after trigger is processed
    uint8_t isSaveable: 1; // save event in deep sleep mode.
} AlarmMode_t;

typedef void (*OnTick_t)();  // alarm callback function typedef

typedef struct
{
    OnTick_t onTickHandler;
    uint32_t value;
    uint64_t nextTrigger;
    AlarmMode_t Mode;
} alarm_t;



static const int year_lengths[2] = {
  365,
  366
};


typedef enum {
    dtMillisecond,
    dtSecond,
    dtMinute,
    dtHour,
    dtDay
} dtUnits_t;




// macro to return true if the given type is a time based alarm, false if timer or not allocated
#define dtIsAlarm(_type_)  (_type_ >= dtExplicitAlarm && _type_ < dtLastAlarmType)
#define dtUseAbsoluteValue(_type_)  (_type_ == dtTimer || _type_ == dtExplicitAlarm)

typedef uint8_t AlarmID_t;
typedef AlarmID_t AlarmId;  // Arduino friendly name


//#define AlarmHMS(_hr_, _min_, _sec_) (_hr_ * SECS_PER_HOUR + _min_ * SECS_PER_MIN + _sec_)
//Hack to get timezone and stuff working



#ifdef ARDUINO_ARCH_ESP8266
#include <functional>
typedef std::function<void()> OnTick_t;
#else
typedef void (*OnTick_t)();  // alarm callback function typedef
#endif

// class containing the collection of alarms
class TimeEventsClass
{
private:
    alarm_t *  m_alarms;
    
    uint8_t isServicing;
    uint8_t servicedAlarmId; // the alarm currently being serviced
    AlarmID_t create(uint32_t value, OnTick_t onTickHandler, uint8_t isOneShot, dtAlarmPeriod_t alarmType);

public:
    TimeEventsClass(alarm_t alarms[]);
    //Translate Alarm time from localtime in to epochtime this will handle timezone things
    int AlarmHMS(int H, int M, int S) {
        time_t t1, t2;
        struct tm tma;
        time(&t1);
        localtime_r(&t1, &tma);
        tma.tm_hour = H;
        tma.tm_min = M;
        tma.tm_sec = S;
        t2 = mktime(&tma);
        int diff = t2 - previousMidnight(t1);
        if (diff > SECS_PER_DAY)
            diff -= SECS_PER_DAY;
        return diff;
    }
    // functions to create alarms and timers

    void loop();

    uint64_t timeNow() {
        return rtc_time_slowclk_to_us(rtc_time_get(), esp_clk_slowclk_cal_get()) / 1000;
    }


    // trigger once at the given time in the future
    AlarmID_t triggerOnce(uint32_t value, OnTick_t onTickHandler) {
        if (value <= 0) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtExplicitAlarm);
    }

    // trigger once at given time of day
    AlarmID_t alarmOnce(uint32_t value, OnTick_t onTickHandler) {
        if (value <= 0 || value > SECS_PER_DAY) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtDailyAlarm);
    }
    AlarmID_t alarmOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return alarmOnce(AlarmHMS(H, M, S), onTickHandler);
    }

    // trigger once on a given day and time
    AlarmID_t alarmOnce(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {
        uint32_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtWeeklyAlarm);
    }

    // trigger daily at given time of day
    AlarmID_t alarmRepeat(uint32_t value, OnTick_t onTickHandler) {
        if (value > SECS_PER_DAY) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtDailyAlarm);
    }
    AlarmID_t alarmRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return alarmRepeat(AlarmHMS(H, M, S), onTickHandler);
    }

    // trigger weekly at a specific day and time
    AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {
        uint32_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtWeeklyAlarm);
    }

    // trigger once after the given number of seconds
    AlarmID_t timerOnce(uint32_t value, OnTick_t onTickHandler) {
        if (value <= 0) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtTimer);
    }
    AlarmID_t timerOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return timerOnce(AlarmHMS(H, M, S), onTickHandler);
    }

    // trigger at a regular interval
    AlarmID_t timerRepeat(uint32_t value, OnTick_t onTickHandler) {
        if (value <= 0) return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtTimer);
    }

    AlarmID_t timerRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return timerRepeat(AlarmHMS(H, M, S), onTickHandler);
    }
    boolean timerReset(AlarmID_t ID);
    void updateNextTrigger(AlarmID_t ID);

    // low level methods
    void enable(AlarmID_t ID);                // enable the alarm to trigger
    void disable(AlarmID_t ID);               // prevent the alarm from triggering
    AlarmID_t getTriggeredAlarmId();          // returns the currently triggered  alarm id
    bool getIsServicing();                    // returns isServicing
    void write(AlarmID_t ID, uint32_t value);   // write the value (and enable) the alarm with the given ID
    uint32_t read(AlarmID_t ID);                // return the value for the given timer
    dtAlarmPeriod_t readType(AlarmID_t ID);   // return the alarm type for the given alarm ID

    void free(AlarmID_t ID);                  // free the id to allow its reuse

#ifndef USE_SPECIALIST_METHODS
private:  // the following methods are for testing and are not documented as part of the standard library
#endif
    uint8_t count();                          // returns the number of allocated timers
    uint64_t getNextTrigger();                  // returns the time of the next scheduled alarm
    bool isAllocated(AlarmID_t ID);           // returns true if this id is allocated
    bool isAlarm(AlarmID_t ID);               // returns true if id is for a time based alarm, false if its a timer or not allocated
};

#endif /* TimeEvents_h */
