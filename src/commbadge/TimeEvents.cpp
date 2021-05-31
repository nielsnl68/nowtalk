/*
  TimeAlarms.cpp - Arduino Time alarms for use with Time library
  Copyright (c) 2008-2011 Michael Margolis.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
 */

 /*
  2 July 2011 - replaced m_alarms types implied from m_alarms value with enums to make trigger logic more robust
              - this fixes bug in repeating weekly alarms - thanks to Vincent Valdy and draythomp for testing
*/

#include "TimeEvents.h"

#define IS_ONESHOT  true   // constants used in arguments to create method
#define IS_REPEAT   false



//**************************************************************
//* Private Methods


//**************************************************************
//* Time Alarms Public Methods

TimeEventsClass::TimeEventsClass(alarm_t alarms[])
{
    m_alarms = alarms;
    isServicing = false;
    for (uint8_t id = 0; id < dtNBR_ALARMS; id++) {
        free(id);   // ensure all Alarms are cleared and available for allocation
    }
}

void TimeEventsClass::enable(AlarmID_t ID)
{
    if (isAllocated(ID)) {
        if ((!(dtUseAbsoluteValue(m_alarms[ID].Mode.alarmType) && (m_alarms[ID].value == 0))) && (m_alarms[ID].onTickHandler != NULL)) {
            // only enable if value is non zero and a tick handler has been set
            // (is not NULL, value is non zero ONLY for dtTimer & dtExplicitAlarm
            // (the rest can have 0 to account for midnight))
            m_alarms[ID].Mode.isEnabled = true;
            updateNextTrigger(ID); // trigger is updated whenever  this is called, even if already enabled
        }
        else {
            m_alarms[ID].Mode.isEnabled = false;
        }
    }
}

void TimeEventsClass::disable(AlarmID_t ID)
{
    if (isAllocated(ID)) {
        m_alarms[ID].Mode.isEnabled = false;
    }
}

// write the given value to the given m_alarms
void TimeEventsClass::write(AlarmID_t ID, uint32_t value)
{
    if (isAllocated(ID)) {
        m_alarms[ID].value = value;  //note: we don't check value as we do it in enable()
        m_alarms[ID].nextTrigger = 0; // clear out previous trigger time (see issue #12)
        enable(ID);  // update trigger time
    }
}

// return the value for the given m_alarms ID
uint32_t TimeEventsClass::read(AlarmID_t ID)
{
    if (isAllocated(ID)) {
        return m_alarms[ID].value;
    }
    else {
        return dtINVALID_TIME;
    }
}

// return the m_alarms type for the given m_alarms ID
dtAlarmPeriod_t TimeEventsClass::readType(AlarmID_t ID)
{
    if (isAllocated(ID)) {
        return (dtAlarmPeriod_t)m_alarms[ID].Mode.alarmType;
    }
    else {
        return dtNotAllocated;
    }
}

void TimeEventsClass::free(AlarmID_t ID)
{
    if (isAllocated(ID)) {
        m_alarms[ID].Mode.isEnabled = false;
        m_alarms[ID].Mode.alarmType = dtNotAllocated;
        m_alarms[ID].onTickHandler = NULL;
        m_alarms[ID].value = 0;
        m_alarms[ID].nextTrigger = 0;
    }
}

// returns the number of allocated timers
uint8_t TimeEventsClass::count()
{
    uint8_t c = 0;
    for (uint8_t id = 0; id < dtNBR_ALARMS; id++) {
        if (isAllocated(id)) c++;
    }
    return c;
}

// returns true only if id is allocated and the type is a time based m_alarms, returns false if not allocated or if its a timer
bool TimeEventsClass::isAlarm(AlarmID_t ID)
{
    return(isAllocated(ID) && dtIsAlarm(m_alarms[ID].Mode.alarmType));
}

// returns true if this id is allocated
bool TimeEventsClass::isAllocated(AlarmID_t ID)
{
    return (ID < dtNBR_ALARMS&& m_alarms[ID].Mode.alarmType != dtNotAllocated);
}

// returns the currently triggered m_alarms id
// returns dtINVALID_ALARM_ID if not invoked from within an m_alarms handler
AlarmID_t TimeEventsClass::getTriggeredAlarmId()
{
    if (isServicing) {
        return servicedAlarmId;  // new private data member used instead of local loop variable i in loop();
    }
    else {
        return dtINVALID_ALARM_ID; // valid ids only available when servicing a callback
    }
}

//returns isServicing
bool TimeEventsClass::getIsServicing()
{
    return isServicing;
}

//***********************************************************
//* Private Methods

void TimeEventsClass::loop()
{
    if (!isServicing) {
        uint64_t now = timeNow();
        isServicing = true;
        for (servicedAlarmId = 0; servicedAlarmId < dtNBR_ALARMS; servicedAlarmId++) {
            if (m_alarms[servicedAlarmId].Mode.isEnabled && (now >= m_alarms[servicedAlarmId].nextTrigger)) {
                OnTick_t TickHandler = m_alarms[servicedAlarmId].onTickHandler;
                if (m_alarms[servicedAlarmId].Mode.isOneShot) {
                    free(servicedAlarmId);  // free the ID if mode is OnShot
                }
                else {
                    updateNextTrigger(servicedAlarmId);
                }
                if (TickHandler != NULL) {
                    TickHandler();     // call the handler
                }
            }
        }
        isServicing = false;
    }
}

// returns the absolute time of the next scheduled m_alarms, or 0 if none
uint64_t TimeEventsClass::getNextTrigger()
{
    uint64_t nextTrigger =0;  // the max time value

    for (uint8_t id = 0; id < dtNBR_ALARMS; id++) {
        if (isAllocated(id)) {
            if ((m_alarms[id].nextTrigger < nextTrigger) || (nextTrigger == 0)) {
                nextTrigger = m_alarms[id].nextTrigger;
            }
        }
    }
  //  Serial.printf("get nextTrigger return: %llx\n", nextTrigger);

    return nextTrigger;
}

// attempt to create an m_alarms and return true if successful
AlarmID_t TimeEventsClass::create(uint32_t value, OnTick_t onTickHandler, uint8_t isOneShot, dtAlarmPeriod_t alarmType)
{
    uint64_t now = timeNow();
    if (!((dtIsAlarm(alarmType) && now < SECS_PER_YEAR) || (dtUseAbsoluteValue(alarmType) && (value == 0)))) {
        // only create m_alarms ids if the time is at least Jan 1 1971
        for (uint8_t id = 0; id < dtNBR_ALARMS; id++) {
            if (m_alarms[id].Mode.alarmType == dtNotAllocated) {
                // here if there is an m_alarms id that is not allocated
                m_alarms[id].onTickHandler = onTickHandler;
                m_alarms[id].Mode.isOneShot = isOneShot;
                m_alarms[id].Mode.alarmType = alarmType;
                m_alarms[id].value = value;
                enable(id);
                return id;  // m_alarms created ok
            }
        }
    }
    return dtINVALID_ALARM_ID; // no IDs available or time is invalid
}

void TimeEventsClass::updateNextTrigger(AlarmID_t ID)
{
    if (m_alarms[ID].Mode.isEnabled) {
        uint64_t now = timeNow();
        if (m_alarms[ID].nextTrigger <= now) {
            switch (m_alarms[ID].Mode.alarmType) {
            case dtTimer:
                /* code */
                m_alarms[ID].nextTrigger = now + m_alarms[ID].value;  // add the value to previous time (this ensures delay always at least Value seconds)
                break;

            case dtExplicitAlarm:
                /* code */
                m_alarms[ID].nextTrigger = m_alarms[ID].value;  // yes, trigger on this value
                if (m_alarms[ID].nextTrigger <= now) free(ID);
                break;

            case  dtDailyAlarm:
                //if this is a daily m_alarms
                if (m_alarms[ID].value + previousMidnight(now) <= now) {
                    // if time has passed then set for tomorrow
                    m_alarms[ID].nextTrigger = m_alarms[ID].value + nextMidnight(now);
                }
                else {
    // set the date to today and add the time given in value
                    m_alarms[ID].nextTrigger = m_alarms[ID].value + previousMidnight(now);
                }
                break;

            case dtWeeklyAlarm:
                // if this is a weekly m_alarms
                if ((m_alarms[ID].value + previousSunday(now)) <= now) {
                    // if day has passed then set for the next week.
                    m_alarms[ID].nextTrigger = m_alarms[ID].value + nextSunday(now);
                }
                else {
                    // set the date to this week today and add the time given in value
                    m_alarms[ID].nextTrigger = m_alarms[ID].value + previousSunday(now);
                }
                break;

            default:
                // its not a recognized m_alarms type - this should not happen
                free(ID);  // Disable the m_alarms
            }
        }
    }
}

boolean TimeEventsClass::timerReset(AlarmID_t ID) {
    if (isAllocated(ID) && m_alarms[ID].Mode.alarmType == dtTimer) {
        m_alarms[ID].nextTrigger = 0;
        updateNextTrigger(ID);
        return true;
    }
    return false;
}