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
 * Updates the alarm state document in Firestore.
 * @param firestore The Firestore instance.
 * @param data The partial data to update.
 */
export function updateAlarmState(firestore: Firestore, data: AlarmStateUpdate) {
  const alarmStateRef = doc(firestore, '/alarm/state');

  // Use setDoc with merge to create the document if it doesn't exist or update it if it does.
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
}
