import { defineConfig } from "vitest/config";

export default defineConfig({
    plugins: [],
    test: {
        typecheck: {
            enabled: true,
        },
        mockReset: true,
        onConsoleLog() {
            return false;
        },
        coverage: {
            enabled: false,
            provider: "v8",
            include: ["src/**"],
            clean: true,
            cleanOnRerun: true,
            reportsDirectory: "coverage",
            reporter: ["text", "html"],
            reportOnFailure: false,
            thresholds: { 100: true },
        },
    },
});
