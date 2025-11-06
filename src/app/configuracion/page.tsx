import MainLayout from "@/components/layout/main-layout";
import ThresholdForm from "@/components/config/threshold-form";

export default function ConfiguracionPage() {
  return (
    <MainLayout>
      <h1 className="text-3xl font-bold mb-6">Configuración de Umbrales</h1>
      <p className="text-muted-foreground mb-8">
        Ajusta los umbrales de los sensores y la lógica de eventos para cambiar la sensibilidad de la detección de estrés.
      </p>
      <ThresholdForm />
    </MainLayout>
  );
}
