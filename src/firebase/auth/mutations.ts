'use client';

import {
  Auth,
  createUserWithEmailAndPassword,
  signInWithEmailAndPassword,
} from 'firebase/auth';

export function signUpWithEmail(auth: Auth, email: string, pass: string) {
  return createUserWithEmailAndPassword(auth, email, pass);
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
    return signInWithEmailAndPassword(auth, email, pass);
}
