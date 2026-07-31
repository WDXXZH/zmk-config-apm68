# SPDX-License-Identifier: MIT

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse-address=0x08000000")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
