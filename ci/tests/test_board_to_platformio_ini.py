import unittest

from ci.boards import Board


class TestBoardToPlatformioIni(unittest.TestCase):
    """Tests for Board.to_platformio_ini()."""

    def _ini_to_set(self, ini: str) -> set[str]:
        """Return a set with each non-empty, stripped line of the ini snippet."""
        return {line.strip() for line in ini.splitlines() if line.strip()}

    def test_basic_fields(self):
        board = Board(board_name="uno", platform="atmelavr", framework="arduino")
        ini = board.to_platformio_ini()
        lines = self._ini_to_set(ini)
        expected = {
            "[env:uno]",
            "board = uno",
            "platform = atmelavr",
            "framework = arduino",
        }
        self.assertTrue(expected.issubset(lines))
        # Should not reference internal attributes
        self.assertNotIn("platform_needs_install", ini)

    def test_real_board_name(self):
        board = Board(
            board_name="esp32c3",
            real_board_name="esp32-c3-devkitm-1",
            platform="espressif32",
        )
        ini = board.to_platformio_ini()
        lines = self._ini_to_set(ini)
        self.assertIn("[env:esp32c3]", lines)
        self.assertIn("board = esp32-c3-devkitm-1", lines)

    def test_flags_and_unflags(self):
        board = Board(
            board_name="custom",
            defines=["FASTLED_TEST=1"],
            build_flags=["-O2"],
        )
        ini = board.to_platformio_ini()
        lines = self._ini_to_set(ini)
        # The build_flags line should contain both the define and the custom flag
        build_flags_line = next(
            (line for line in lines if line.startswith("build_flags")), None
        )
        self.assertIsNotNone(build_flags_line)
        assert build_flags_line is not None  # hint for type checker
        self.assertIn("-DFASTLED_TEST=1", build_flags_line)
        self.assertIn("-O2", build_flags_line)


if __name__ == "__main__":
    unittest.main()
