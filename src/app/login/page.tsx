import { LoginForm } from "@/components/auth/login-form";
import { Shield } from "lucide-react";
import Link from "next/link";

export default function LoginPage() {
  return (
    <div className="flex flex-col items-center justify-center min-h-screen bg-background">
      <div className="w-full max-w-md p-8 space-y-8">
        <div className="flex flex-col items-center">
            <Shield className="h-10 w-10 text-primary" />
            <h1 className="text-3xl font-bold mt-4">Bienvenido de Nuevo</h1>
            <p className="text-muted-foreground">Inicia sesión en tu cuenta de SafePlace</p>
        </div>
        <LoginForm />
        <p className="text-center text-sm text-muted-foreground">
          ¿No tienes una cuenta?{" "}
          <Link
            href="/signup"
            className="font-semibold text-primary hover:underline"
          >
            Regístrate
          </Link>
        </p>
      </div>
    </div>
  );
}
