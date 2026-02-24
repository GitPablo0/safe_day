
'use client';

import {
  Auth,
  createUserWithEmailAndPassword,
  signInWithEmailAndPassword,
  UserCredential,
} from 'firebase/auth';
import { getFirestore, doc, setDoc } from 'firebase/firestore';

/**
 * Registra un nuevo cuidador.
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
      
      // Inicializar perfil mínimo sin volumen
      await setDoc(doc(db, `users/${user.uid}/settings/device`), {
        status: 'online'
      });
    } catch (dbError) {
      console.error('Error al inicializar SafeDay:', dbError);
    }
  }

  return userCredential;
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
  return signInWithEmailAndPassword(auth, email, pass);
}
