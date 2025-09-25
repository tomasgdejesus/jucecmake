/// <reference types="vite/client" />
declare module 'juce-framework-frontend' {
  export function getNativeFunction(name: string): function;
}