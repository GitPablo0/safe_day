import { LoginForm } from "@/components/auth/login-form";
import { BrainCircuit } from "lucide-react";
import Link from "next/link";

export default function LoginPage() {
  return (
    <div className="flex flex-col items-center justify-center min-h-screen bg-background">
      <div className="w-full max-w-md p-8 space-y-8">
        <div className="flex flex-col items-center">
            <BrainCircuit className="h-10 w-10 text-primary" />
            <h1 className="text-3xl font-bold mt-4">Welcome Back</h1>
            <p className="text-muted-foreground">Sign in to your NeuroSense account</p>
        </div>
        <LoginForm />
        <p className="text-center text-sm text-muted-foreground">
          Don't have an account?{" "}
          <Link
            href="/signup"
            className="font-semibold text-primary hover:underline"
          >
            Sign Up
          </Link>
        </p>
      </div>
    </div>
  );
}
