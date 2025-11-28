'use client';

import { doc, setDoc, Firestore } from 'firebase/firestore';
import { ref, set, Database } from 'firebase/database';
import { errorEmitter } from '@/firebase/error-emitter';
import { FirestorePermissionError } from '@/firebase/errors';

interface AlarmStateUpdate {
    isArmed?: boolean;
    presenceDetected?: boolean;
    alarmDuration?: number;
}

/**
 * Updates the alarm state document in Firestore and Realtime Database.
 * @param firestore The Firestore instance.
 * @param rtdb The Realtime Database instance.
 * @param data The partial data to update.
 */
export function updateAlarmState(firestore: Firestore, rtdb: Database, data: AlarmStateUpdate) {
  const alarmStateRef = doc(firestore, '/alarm/state');
  const rtdbRef = ref(rtdb, '/alarm/state');

  // 1. Update Firestore (for the web app's real-time updates)
  setDoc(alarmStateRef, data, { merge: true })
    .catch((serverError) => {
      const permissionError = new FirestorePermissionError({
        path: alarmStateRef.path,
        operation: 'write',
        requestResourceData: data,
      });

      // Emit the error for centralized handling.
      errorEmitter.emit('permission-error', permissionError);
    });

  // 2. Update Realtime Database (for the ESP32's real-time updates)
  // We use set() which will create/update the path with the new data.
  // We get the current state and merge it with the new data to mimic `merge: true`.
  const { get, child } = require("firebase/database");
  get(child(ref(rtdb), '/alarm/state')).then((snapshot) => {
    const currentState = snapshot.val() || {};
    const newState = { ...currentState, ...data };
    set(rtdbRef, newState).catch((error) => {
        console.error("Error writing to Realtime Database: ", error);
        // Optional: Handle RTDB write errors, e.g., show a toast
    });
  });
}
