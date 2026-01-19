import { beforeAll, describe, expect, it } from "vitest";
import type { EzspNative } from "../src/index.js";

describe("EZSP Lifecycle", () => {
    let binding: EzspNative;

    beforeAll(async () => {
        binding = (await import("../src/index.js")).default;
    });

    describe("start", () => {
        it("prevents starting before init", () => {
            expect(() => {
                binding.start();
            }).toThrow();
        });
    });

    describe.skip("stop", () => {
        // TODO: mock hardware
    });
});
