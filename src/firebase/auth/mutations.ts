'use client';

import {
  Auth,
  createUserWithEmailAndPassword,
  signInWithEmailAndPassword,
  UserCredential,
} from 'firebase/auth';
import { getFirestore, doc, setDoc } from 'firebase/firestore';

/**
 * Creates a new user with email and password, and then creates their
 * initial alarm state document in Firestore.
 * @param auth The Firebase Auth instance.
 * @param email The user's email.
 * @param pass The user's password.
 * @returns A promise that resolves with the UserCredential on successful creation of both user and document.
 */
export async function signUpWithEmail(
  auth: Auth,
  email: string,
  pass: string
): Promise<UserCredential> {
  const userCredential = await createUserWithEmailAndPassword(auth, email, pass);
  const user = userCredential.user;

  if (user) {
    try {
      // It's crucial to get the Firestore instance associated with the same app
      const db = getFirestore(auth.app);
      const alarmStateRef = doc(db, `users/${user.uid}/alarm/state`);

      // Create the complete initial document
      const initialAlarmState = {
        isArmed: false,
        presenceDetected: false,
        alarmDuration: 10, // A sensible default
      };

      // Use setDoc to create the document. Since it's a new user, no merge is needed,
      // but using it doesn't hurt and makes the pattern consistent.
      await setDoc(alarmStateRef, initialAlarmState);
    } catch (dbError) {
      // If creating the doc fails, we must handle this.
      // For now, log the error but let the user creation succeed.
      // In a production app, you might want to delete the user.
      console.error(
        'User created, but failed to create initial alarm state:',
        dbError
      );
      // Re-throwing the error will make it visible in the UI's catch block
      throw new Error('User created, but failed to initialize database state.');
    }
  }

  return userCredential;
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
  return signInWithEmailAndPassword(auth, email, pass);
}
