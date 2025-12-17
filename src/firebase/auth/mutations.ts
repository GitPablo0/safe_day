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
 * initial sound alarm state document in Firestore.
 * @param auth The Firebase Auth instance.
 * @param email The user's email.
 * @param pass The user's password.
 * @returns A promise that resolves with the UserCredential.
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
      const alarmStateRef = doc(db, `users/${user.uid}/soundAlarm/state`);

      const initialAlarmState = {
        isArmed: false,
        soundDetected: false,
      };

      await setDoc(alarmStateRef, initialAlarmState);
    } catch (dbError) {
      console.error(
        'User created, but failed to create initial sound alarm state:',
        dbError
      );
      throw new Error('User created, but failed to initialize database state.');
    }
  }

  return userCredential;
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
  return signInWithEmailAndPassword(auth, email, pass);
}
