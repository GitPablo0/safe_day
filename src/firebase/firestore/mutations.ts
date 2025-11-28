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
 * @param firestore The Firestore instance.
 * @param userId The UID of the user whose alarm state is being updated.
 * @param data The partial data to update.
 */
export function updateAlarmState(firestore: Firestore, userId: string, data: AlarmStateUpdate) {
  const docRef = doc(firestore, `users/${userId}/alarm/state`);

  // Use setDoc with merge: true to update the document without overwriting it.
  // This is a non-blocking operation.
  setDoc(docRef, data, { merge: true }).catch((error) => {
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
