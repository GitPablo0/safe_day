import { SignupForm } from "@/components/auth/signup-form";
import { Shield } from "lucide-react";
import Link from "next/link";

export default function SignupPage() {
  return (
    <div className="flex flex-col items-center justify-center min-h-screen bg-background">
      <div className="w-full max-w-md p-8 space-y-8">
        <div className="flex flex-col items-center">
          <Shield className="h-10 w-10 text-primary" />
          <h1 className="text-3xl font-bold mt-4">Crear una Cuenta</h1>
          <p className="text-muted-foreground">
            Comienza a usar SafePlace hoy
          </p>
        </div>
        <SignupForm />
        <p className="text-center text-sm text-muted-foreground">
          ¿Ya tienes una cuenta?{" "}
          <Link
            href="/login"
            className="font-semibold text-primary hover:underline"
          >
            Iniciar Sesión
          </Link>
        </p>
      </div>
    </div>
  );
}
