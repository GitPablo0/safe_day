import { SignupForm } from "@/components/auth/signup-form";
import { BrainCircuit } from "lucide-react";
import Link from "next/link";

export default function SignupPage() {
  return (
    <div className="flex flex-col items-center justify-center min-h-screen bg-background">
      <div className="w-full max-w-md p-8 space-y-8">
        <div className="flex flex-col items-center">
          <BrainCircuit className="h-10 w-10 text-primary" />
          <h1 className="text-3xl font-bold mt-4">Create an Account</h1>
          <p className="text-muted-foreground">
            Get started with NeuroSense today
          </p>
        </div>
        <SignupForm />
        <p className="text-center text-sm text-muted-foreground">
          Already have an account?{" "}
          <Link
            href="/login"
            className="font-semibold text-primary hover:underline"
          >
            Log In
          </Link>
        </p>
      </div>
    </div>
  );
}
