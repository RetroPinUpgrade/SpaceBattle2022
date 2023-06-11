/**************************************************************************
 *     This file is part of the WOS for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    WOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    WOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#ifndef WOS_CONFIG_H

#define USE_WAV_TRIGGER_1p3

#define WOS_NUM_DIGITS  6
#define WOS_MAX_DISPLAY_SCORE 999999
#define WOS_ALL_DIGITS_MASK 0x3F

#define WOS_TYPE_1_SOUND

#define WOS_CREDITS_EEPROM_BYTE          5
#define WOS_HIGHSCORE_EEPROM_START_BYTE  1
#define WOS_AWARD_SCORE_1_EEPROM_START_BYTE      10
#define WOS_AWARD_SCORE_2_EEPROM_START_BYTE      14
#define WOS_AWARD_SCORE_3_EEPROM_START_BYTE      18
#define WOS_TOTAL_PLAYS_EEPROM_START_BYTE        26
#define WOS_TOTAL_REPLAYS_EEPROM_START_BYTE      30
#define WOS_TOTAL_HISCORE_BEATEN_START_BYTE      34
#define WOS_CHUTE_2_COINS_START_BYTE             38
#define WOS_CHUTE_1_COINS_START_BYTE             42
#define WOS_CHUTE_3_COINS_START_BYTE             46
#define WOS_CPC_CHUTE_1_SELECTION_BYTE           50
#define WOS_CPC_CHUTE_2_SELECTION_BYTE           51
#define WOS_CPC_CHUTE_3_SELECTION_BYTE           52

#define RPU_VERSION 2

#define WOS_CONFIG_H
#endif
