"use client"

import * as React from "react"
import * as SliderPrimitive from "@radix-ui/react-slider"

import { cn } from "@/lib/utils"

const Slider = React.forwardRef<
  React.ElementRef<typeof SliderPrimitive.Root>,
  React.ComponentPropsWithoutRef<typeof SliderPrimitive.Root>
>(({ className, ...props }, ref) => (
  <SliderPrimitive.Root
    ref={ref}
    className={cn(
      "relative flex w-full touch-none select-none items-center group",
      className
    )}
    {...props}
  >
    <SliderPrimitive.Track className="relative h-2 w-full grow overflow-hidden rounded-full bg-secondary group-data-[color=amber]:bg-amber-500/30 group-data-[color=yellow]:bg-yellow-400/30 group-data-[color=destructive]:bg-destructive/30">
      <SliderPrimitive.Range className="absolute h-full bg-primary group-data-[color=amber]:bg-amber-500 group-data-[color=yellow]:bg-yellow-400 group-data-[color=destructive]:bg-destructive" />
    </SliderPrimitive.Track>
    <SliderPrimitive.Thumb className="block h-5 w-5 rounded-full border-2 border-primary bg-background ring-offset-background transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:pointer-events-none disabled:opacity-50 group-data-[color=amber]:border-amber-500 group-data-[color=yellow]:border-yellow-400 group-data-[color=destructive]:border-destructive" />
  </SliderPrimitive.Root>
))
Slider.displayName = SliderPrimitive.Root.displayName

export { Slider }
