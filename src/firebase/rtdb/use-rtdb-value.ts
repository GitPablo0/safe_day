'use client';

import { useState, useEffect } from 'react';
import { onValue, off, DatabaseReference } from 'firebase/database';

export interface UseRtdbValueResult<T> {
  data: T | null;
  isLoading: boolean;
  error: Error | null;
}

/**
 * React hook to subscribe to a Firebase Realtime Database path.
 *
 * @template T - The expected type of the data at the given path.
 * @param {DatabaseReference | null} dbRef - The Firebase Realtime Database reference to listen to. If null, the hook will not fetch data.
 * @returns {UseRtdbValueResult<T>} An object containing the data, loading state, and error.
 */
export function useRtdbValue<T = any>(
  dbRef: DatabaseReference | null
): UseRtdbValueResult<T> {
  const [data, setData] = useState<T | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(true);
  const [error, setError] = useState<Error | null>(null);

  useEffect(() => {
    if (!dbRef) {
      setIsLoading(false);
      setData(null);
      return;
    }

    setIsLoading(true);

    const onValueChange = onValue(
      dbRef,
      (snapshot) => {
        setData(snapshot.val() as T);
        setIsLoading(false);
        setError(null);
      },
      (err) => {
        console.error(
          `Error fetching RTDB value from path: ${dbRef.toString()}`,
          err
        );
        setError(err);
        setIsLoading(false);
        setData(null);
      }
    );

    // Cleanup function to remove the listener when the component unmounts
    // or when the dbRef changes.
    return () => {
      off(dbRef, 'value', onValueChange);
    };
  }, [dbRef]); // Re-run the effect if the database reference changes.

  return { data, isLoading, error };
}
