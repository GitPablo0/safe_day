'use client';

import { doc, setDoc, Firestore } from 'firebase/firestore';
import { errorEmitter } from '@/firebase/error-emitter';
import { FirestorePermissionError } from '@/firebase/errors';

interface AlarmStateUpdate {
  isArmed?: boolean;
  presenceDetected?: boolean;
  alarmDuration?: number;
}

/**
 * Updates the alarm state in Firestore for a specific user.
 * Creates the document with default values if it doesn't exist, thanks to { merge: true }.
 * @param firestore The Firestore instance.
 * @param userId The UID of the user whose alarm state is being updated.
 * @param data The partial data to update.
 */
export function updateAlarmState(firestore: Firestore, userId: string, data: AlarmStateUpdate) {
  const docRef = doc(firestore, `users/${userId}/alarm/state`);

  // Define the data to be written. If we are arming the alarm, always reset presenceDetected.
  const updateData = { ...data };
  if (updateData.isArmed === true) {
    updateData.presenceDetected = false;
  } else if (updateData.isArmed === false) {
    // If disarming, also reset presenceDetected
    updateData.presenceDetected = false;
  }


  // Use setDoc with merge: true. This is the key. It will CREATE the document
  // with the provided data if it doesn't exist, or UPDATE the fields if it does.
  setDoc(docRef, updateData, { merge: true }).catch((error) => {
    // Construct a rich, contextual error and emit it globally.
    const permissionError = new FirestorePermissionError({
      path: docRef.path,
      operation: 'update',
      requestResourceData: data,
    });
    errorEmitter.emit('permission-error', permissionError);

    // Also log the original server error for debugging purposes if needed,
    // but the primary error handling is through the emitter.
    console.error('Firestore write failed:', error);
  });
}
