'use client';

import { useContext } from 'react';
import { FirebaseApp } from 'firebase/app';
import { Firestore } from 'firebase/firestore';
import { Auth, User } from 'firebase/auth';
import { Database } from 'firebase/database';
import { FirebaseContext, FirebaseContextState } from '@/firebase/provider';

// Return type for useFirebase()
export interface FirebaseServices {
  firebaseApp: FirebaseApp;
  firestore: Firestore;
  auth: Auth;
  rtdb: Database;
}

// Return type for useUser() - specific to user auth state
export interface UserHookResult {
  user: User | null;
  isUserLoading: boolean;
  userError: Error | null;
}

/**
 * Hook to access core Firebase services.
 * Throws error if used outside of FirebaseProvider or if services are not available.
 */
export const useFirebase = (): FirebaseServices => {
  const context = useContext(FirebaseContext);

  if (
    context === undefined ||
    !context.firebaseApp ||
    !context.firestore ||
    !context.auth ||
    !context.rtdb
  ) {
    throw new Error('useFirebase must be used within a FirebaseProvider.');
  }

  return {
    firebaseApp: context.firebaseApp,
    firestore: context.firestore,
    auth: context.auth,
    rtdb: context.rtdb,
  };
};

/** Hook to access Firebase Auth instance. */
export const useAuth = (): Auth => {
  const context = useContext(FirebaseContext);
  if (context === undefined || !context.auth) {
    throw new Error('useAuth must be used within a FirebaseProvider.');
  }
  return context.auth;
};

/** Hook to access Firestore instance. */
export const useFirestore = (): Firestore => {
  const context = useContext(FirebaseContext);
  if (context === undefined || !context.firestore) {
    throw new Error('useFirestore must be used within a FirebaseProvider.');
  }
  return context.firestore;
};

/** Hook to access Firebase App instance. */
export const useFirebaseApp = (): FirebaseApp => {
  const context = useContext(FirebaseContext);
  if (context === undefined || !context.firebaseApp) {
    throw new Error('useFirebaseApp must be used within a FirebaseProvider.');
  }
  return context.firebaseApp;
};

/** Hook to access Realtime Database instance. */
export const useRtdb = (): Database => {
  const context = useContext(FirebaseContext);
  if (context === undefined || !context.rtdb) {
    throw new Error('useRtdb must be used within a FirebaseProvider.');
  }
  return context.rtdb;
};

/**
 * Hook specifically for accessing the authenticated user's state.
 * This provides the User object, loading status, and any auth errors.
 * @returns {UserHookResult} Object with user, isUserLoading, userError.
 */
export const useUser = (): UserHookResult => {
  const context = useContext(FirebaseContext);
  if (context === undefined) {
    throw new Error('useUser must be used within a FirebaseProvider.');
  }
  return {
    user: context.user,
    isUserLoading: context.isUserLoading,
    userError: context.userError,
  };
};
