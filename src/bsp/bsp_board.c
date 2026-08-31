/**
 * bsp_board.c —— 板级选择层
 *
 * 通过 BSP_BOARD_SELECT 宏一键切换当前板子。
 * 新增板子时：
 *   1. bsp.h 增加 BSP_BOARD_XXX
 *   2. 新建 bsp_xxx.h/.c，提供 bsp_xxx_board_init/get
 *   3. 在下面 #elif 分支加一行
 *   4. 修改 BSP_BOARD_SELECT 即可切换
 */
#include "bsp.h"

#if BSP_BOARD_SELECT == BSP_BOARD_CH579
#include "bsp_ch579.h"
static uint8_t board_init(void) { return bsp_ch579_board_init(); }
static bsp_t *board_get(void)   { return bsp_ch579_board_get(); }

#elif BSP_BOARD_SELECT == BSP_BOARD_MEIDI
/* TODO: bsp_meidi.c 实现后打开 */
#error "BSP_BOARD_MEIDI not implemented yet"

#elif BSP_BOARD_SELECT == BSP_BOARD_TOSHIBA
/* TODO: bsp_toshiba.c 实现后打开 */
#error "BSP_BOARD_TOSHIBA not implemented yet"

#elif BSP_BOARD_SELECT == BSP_BOARD_HAIER
/* TODO: bsp_haier.c 实现后打开 */
#error "BSP_BOARD_HAIER not implemented yet"

#else
#error "Unknown BSP_BOARD_SELECT"
#endif

uint8_t bsp_board_init(void)
{
    return board_init();
}

bsp_t *bsp_board_get(void)
{
    return board_get();
}
