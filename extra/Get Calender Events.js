/* Fetch first event of the day to set alarm clock
 * Platform: Google WebApp Script
 * (c) Copyright 2016, Coert Vonk, Sander Vonk
 */

function _alarmTime(event) {
  if (event == undefined ) {
    return undefined;
  }
  // https://code.google.com/p/google-apps-script-issues/issues/detail?id=4433
  var reminders = event.getPopupReminders();  // known issue: method getPopupReminders() doesn't return anything if there is only one reminder so try setting two reminders for the same timed
  if (reminders.length == 0) {
    reminder = 0;   
  }
  var longestReminder = 0;
  for (var ii=0; ii < reminders.length; ii++) {
    if (reminders[ii] > longestReminder) {
      longestReminder = reminders[ii];
    }
  }
  
  date = new Date(event.getStartTime().getTime()-longestReminder*60*1000);
  return date;
}

function _findFirstAlarm(events) {
  var firstAlarmTime = undefined;
  var firstAlarmIdx = undefined;
  
  for (var ii = 0; ii < events.length; ii++) {
    var event=events[ii];
    switch(event.getMyStatus()) {
      case CalendarApp.GuestStatus.OWNER:
      case CalendarApp.GuestStatus.YES:
      case CalendarApp.GuestStatus.MAYBE:
          var alarmTime = _alarmTime(event);
          if (firstAlarmTime == undefined || alarmTime < firstAlarmTime) {
            firstAlarmIdx = ii;
            firstAlarmTime = alarmTime;
        }
        break;
      default:
        break;
    }
  }
  if (firstAlarmIdx == undefined) {
    return undefined;
  }
  return events[firstAlarmIdx];
}

function doGet(e) {

  // open calendar

  var cal = CalendarApp.getCalendarById('your.e.mail@gmail.com');
  if (cal == undefined) {
    return ContentService.createTextOutput("no access to calendar");
  }

  // find the first event today that I'm participating in, and that is not an all day event.

  const now = new Date();
  var todayStart = new Date(); todayStart.setHours(0, 0, 0);  // start at midnight this day
  const oneday = 24*3600000; // [msec]
  const todayStop = new Date(todayStart.getTime() + oneday - 1);
  var eventsToday = cal.getEvents(todayStart, todayStop);
  var firstAlarmToday = _findFirstAlarm(eventsToday);
  
  // find the first event tomorrow that I'm participating in, and that is not an all day event.

  const tomorrowStart = new Date(todayStart.getTime() + oneday);
  const tomorrowStop = new Date(tomorrowStart.getTime() + oneday - 1);
  var eventsTomorrow = cal.getEvents(tomorrowStart, tomorrowStop);
  var firstAlarmTomorrow = _findFirstAlarm(eventsTomorrow);
  
  // select the alarm event

  const event = firstAlarmToday != undefined && _alarmTime(firstAlarmToday) > now ? firstAlarmToday : firstAlarmTomorrow;

  // print event details that should trigger alarm

  var str = '';
  if ( event != undefined ) {
    const date = event.getStartTime();
    const alarm = _alarmTime(event);
    const startMinutes = date.getHours()*60 + date.getMinutes();
    const alarmMinutes = alarm.getHours()*60 + alarm.getMinutes();
    
    str += alarmMinutes + '\n' +   // alarm [minutes since midnight]
      startMinutes + '\n' +   // start time of event in [minutes since midnight]
        event.getTitle() +'\n'; // event title
    
  }
  Logger.log("<REPLY>" + str + "</REPLY>");
  
  return ContentService.createTextOutput(str);
}
