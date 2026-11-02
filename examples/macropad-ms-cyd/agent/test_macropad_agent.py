"""Pruebas del programa USB de MacroPad MS (no pulsan teclas). Uso: python -m unittest"""

import unittest

import macropad_agent as agent


class LineFormat(unittest.TestCase):
    def test_accepts_firmware_lines(self):
        m = agent.LINE.match("MSR_KEY kind=key mod=3 code=16")
        self.assertEqual(m.groups(), ("key", "3", "16"))
        self.assertTrue(agent.LINE.match("MSR_KEY kind=media mod=0 code=205"))

    def test_rejects_anything_else(self):
        for text in ("MSR_KEY kind=run mod=0 code=4", "MSR_KEY kind=key mod=0 code=4; calc",
                     "hello", "MSR_KEY kind=key mod=-1 code=4", "MSR_MACROPAD_BOOT"):
            self.assertIsNone(agent.LINE.match(text), text)


class Mapping(unittest.TestCase):
    def test_every_firmware_action_is_allowed(self):
        keys = [0x10, 0x12, 0x0E, 0x04, 0x19, 0x07, 0x06, 0x1D, 0x16, 0x0F] + list(range(0x68, 0x6E))
        for code in keys:
            self.assertIn(code, agent.KEYS)
        for code in (0xCD, 0xB5, 0xB6, 0xE2, 0xE9, 0xEA):
            self.assertIn(code, agent.MEDIA)

    def test_letters_and_function_keys(self):
        self.assertEqual(agent.KEYS[0x04], ord("A"))
        self.assertEqual(agent.KEYS[0x1D], ord("Z"))
        self.assertEqual(agent.KEYS[0x68], 0x7C)  # F13

    def test_labels(self):
        self.assertEqual(agent.describe("key", 0x03, 0x10), "Ctrl+Mayús+M")
        self.assertEqual(agent.describe("key", 0x08, 0x0F), "Win+L")
        self.assertEqual(agent.describe("key", 0, 0x6B), "F16")
        self.assertEqual(agent.describe("media", 0, 0xCD), "Play/Pausa")


if __name__ == "__main__":
    unittest.main()
