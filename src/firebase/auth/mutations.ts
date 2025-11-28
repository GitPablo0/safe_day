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
      const db = getFirestore(auth.app);
      const alarmStateRef = doc(db, `users/${user.uid}/alarm/state`);

      const initialAlarmState = {
        isArmed: false,
        presenceDetected: false,
        alarmDuration: 10, // Default duration
      };

      // Set the initial document for the new user.
      await setDoc(alarmStateRef, initialAlarmState);
    } catch (dbError) {
      // If creating the doc fails, we might want to handle this.
      // For now, we log the error but let the user creation succeed.
      console.error(
        'User created, but failed to create initial alarm state:',
        dbError
      );
      // Depending on requirements, you might want to delete the user here
      // await user.delete();
      // and re-throw the error to be caught by the UI.
    }
  }

  return userCredential;
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
  return signInWithEmailAndPassword(auth, email, pass);
}
