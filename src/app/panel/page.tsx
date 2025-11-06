import MainLayout from "@/components/layout/main-layout";
import Dashboard from "@/components/dashboard/dashboard";

export default function PanelPage() {
  return (
    <MainLayout>
        <div className="container mx-auto py-8">
            <h1 className="text-3xl font-bold mb-6">Panel de Control</h1>
            <Dashboard />
        </div>
    </MainLayout>
  );
}
