
'use client';

import {
  Auth,
  createUserWithEmailAndPassword,
  signInWithEmailAndPassword,
  UserCredential,
} from 'firebase/auth';
import { getFirestore, doc, setDoc } from 'firebase/firestore';

/**
 * Registra un nuevo cuidador e inicializa su perfil de SafeDay.
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
      
      // Inicializar settings del dispositivo
      await setDoc(doc(db, `users/${user.uid}/settings/device`), {
        volume: 20,
        isMuted: false
      });

      // Crear tarea de bienvenida en la rutina
      const welcomeTaskId = crypto.randomUUID();
      await setDoc(doc(db, `users/${user.uid}/routine/${welcomeTaskId}`), {
        id: welcomeTaskId,
        title: 'Primeros pasos con SafeDay',
        time: '09:00',
        trackNumber: 1,
        isActive: true
      });

    } catch (dbError) {
      console.error('Error inicializando SafeDay:', dbError);
    }
  }

  return userCredential;
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
  return signInWithEmailAndPassword(auth, email, pass);
}
