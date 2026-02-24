
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
      
      // 1. Inicializar settings del dispositivo para el nuevo usuario
      await setDoc(doc(db, `users/${user.uid}/settings/device`), {
        volume: 15,
        isMuted: false
      });

      // 2. Crear una tarea de rutina inicial como ejemplo/bienvenida
      const initialTaskId = crypto.randomUUID();
      await setDoc(doc(db, `users/${user.uid}/routine/${initialTaskId}`), {
        id: initialTaskId,
        title: '¡Bienvenido a SafeDay!',
        time: '08:00',
        trackNumber: 1,
        isActive: true
      });

    } catch (dbError) {
      console.error('Error al inicializar datos de SafeDay:', dbError);
    }
  }

  return userCredential;
}

export function signInWithEmail(auth: Auth, email: string, pass: string) {
  return signInWithEmailAndPassword(auth, email, pass);
}
