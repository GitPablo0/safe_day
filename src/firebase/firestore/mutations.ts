'use client';

import { doc, setDoc, Firestore } from 'firebase/firestore';
import { errorEmitter } from '@/firebase/error-emitter';
import { FirestorePermissionError } from '@/firebase/errors';

interface AlarmStateUpdate {
  isArmed?: boolean;
  soundDetected?: boolean;
  alarmDuration?: number;
}

/**
 * Updates the sound alarm state in Firestore for a specific user.
 * Creates the document with default values if it doesn't exist.
 * @param firestore The Firestore instance.
 * @param userId The UID of the user whose alarm state is being updated.
 * @param data The partial data to update.
 */
export function updateAlarmState(firestore: Firestore, userId: string, data: AlarmStateUpdate) {
  const docRef = doc(firestore, `users/${userId}/soundAlarm/state`);

  const updateData = { ...data };
  if (updateData.isArmed === true || updateData.isArmed === false) {
    updateData.soundDetected = false;
  }

  setDoc(docRef, updateData, { merge: true }).catch((error) => {
    const permissionError = new FirestorePermissionError({
      path: docRef.path,
      operation: 'update',
      requestResourceData: data,
    });
    errorEmitter.emit('permission-error', permissionError);

    console.error('Firestore write failed:', error);
  });
}
