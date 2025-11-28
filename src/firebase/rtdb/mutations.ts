'use client';

import { ref, update, Database, get, child } from 'firebase/database';

interface AlarmStateUpdate {
    isArmed?: boolean;
    presenceDetected?: boolean;
    alarmDuration?: number;
}

/**
 * Updates the alarm state in Realtime Database.
 * It merges the new data with the existing data at the '/alarm/state' path.
 * @param rtdb The Realtime Database instance.
 * @param data The partial data to update.
 */
export async function updateAlarmState(rtdb: Database, data: AlarmStateUpdate) {
  const rtdbRef = ref(rtdb, '/alarm/state');

  try {
    // Firebase RTDB's `update` function works like a `merge: true` by default
    // at the specified path. It only updates the keys provided in the `data` object.
    await update(rtdbRef, data);
  } catch (error) {
      console.error("Error writing to Realtime Database: ", error);
      // Optional: Handle RTDB write errors, e.g., show a toast to the user.
  }
}
