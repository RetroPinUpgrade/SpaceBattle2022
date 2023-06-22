/**************************************************************************
    Space Battle 2022 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

#include "RPU_config.h"
#include "RPU.h"
#include "SpaceBattle2022.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>


#define USE_SCORE_OVERRIDES

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
#include "SendOnlyWavTrigger.h"
SendOnlyWavTrigger wTrig;             // Our WAV Trigger object
#endif

#define SPACE_BATTLE_MAJOR_VERSION  2022
#define SPACE_BATTLE_MINOR_VERSION  2
#define DEBUG_MESSAGES  0


void PlaySoundEffect(unsigned int soundEffectNum, int gain = 1000, boolean overrideSelector = false);
void PlayBackgroundSong(unsigned int songNum, unsigned long backgroundSongNumSeconds = 0);

/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110

#define MACHINE_STATE_ADJUST_FREEPLAY             -21
#define MACHINE_STATE_ADJUST_BALL_SAVE            -22
#define MACHINE_STATE_ADJUST_SOUND_SELECTOR       -23
#define MACHINE_STATE_ADJUST_MUSIC_VOLUME         -24
#define MACHINE_STATE_ADJUST_SFX_VOLUME           -25
#define MACHINE_STATE_ADJUST_CALLOUTS_VOLUME      -26
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING   -27
#define MACHINE_STATE_ADJUST_TILT_WARNING         -28
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE       -29
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE       -30
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES     -31
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD     -32
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD        -33
#define MACHINE_STATE_ADJUST_GOALS_UNTIL_WIZARD   -34
#define MACHINE_STATE_ADJUST_WIZARD_TIME          -35
#define MACHINE_STATE_ADJUST_IDLE_MODE            -36
#define MACHINE_STATE_ADJUST_COMBOS_TO_FINISH     -37
#define MACHINE_STATE_ADJUST_SPINNER_ACCELERATORS -38
#define MACHINE_STATE_ADJUST_ALLOW_RESET          -39
#define MACHINE_STATE_ADJUST_DONE                 -40

// The lower 4 bits of the Game Mode are modes, the upper 4 are for frenzies
// and other flags that carry through different modes
#define GAME_MODE_SKILL_SHOT                        0
#define GAME_MODE_UNSTRUCTURED_PLAY                 1
#define GAME_MODE_BATTLE_START                      2
#define GAME_MODE_BATTLE                            3
#define GAME_MODE_BATTLE_ADD_ENEMY                  4
#define GAME_MODE_BATTLE_WON                        5
#define GAME_MODE_BATTLE_LOST                       6
#define GAME_MODE_INVASION_START                    7
#define GAME_MODE_INVASION                          8
#define GAME_MODE_INVASION_WON                      9
#define GAME_MODE_INVASION_LOST                     10
#define GAME_MODE_WIZARD_START                      11
#define GAME_MODE_WIZARD_WAIT_FOR_BALL              12
#define GAME_MODE_WIZARD                            13
#define GAME_MODE_WIZARD_FINISHED_100               14
#define GAME_MODE_WIZARD_FINISHED_50                15
#define GAME_MODE_WIZARD_FINISHED_10                16
#define GAME_MODE_WIZARD_END_BALL_COLLECT           17
#define GAME_BASE_MODE                              0x1F


#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_SOUND_SELECTOR_BYTE      102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_SFX_VOLUME_BYTE          108
#define EEPROM_MUSIC_VOLUME_BYTE        109
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_CALLOUTS_VOLUME_BYTE     111
#define EEPROM_GOALS_UNTIL_WIZ_BYTE     112
#define EEPROM_IDLE_MODE_BYTE           113
#define EEPROM_WIZ_TIME_BYTE            114
#define EEPROM_SPINNER_ACCELERATOR_BYTE 115
#define EEPROM_COMBOS_GOAL_BYTE         116
#define EEPROM_ALLOW_RESET_BYTE         117
#define EEPROM_EXTRA_BALL_SCORE_UL      140
#define EEPROM_SPECIAL_SCORE_UL         144


#define SOUND_EFFECT_NONE               0
#define SOUND_EFFECT_BONUS_COUNT        1
#define SOUND_EFFECT_OUTLANE_UNLIT      4
#define SOUND_EFFECT_OUTLANE_LIT        6
#define SOUND_EFFECT_BUMPER_HIT         7
#define SOUND_EFFECT_LOWER_BUMPER_HIT   8
//#define SOUND_EFFECT_WAITING_FOR_SKILL  9
//#define SOUND_EFFECT_ADD_CREDIT         10
#define SOUND_EFFECT_TOP_LANE_REPEAT    11
#define SOUND_EFFECT_TOP_LANE_NEW       12
#define SOUND_EFFECT_TOP_LANE_LEVEL_FINISHED  13
#define SOUND_EFFECT_SW_LETTER_AWARDED  14
#define SOUND_EFFECT_DROP_TARGET_HIT    15
#define SOUND_EFFECT_DROP_TARGET_RESET  16
#define SOUND_EFFECT_BALL_OVER          19
#define SOUND_EFFECT_GAME_OVER          20
#define SOUND_EFFECT_FRENZY_BUMPER_HIT  21
#define SOUND_EFFECT_SKILL_SHOT         25
#define SOUND_EFFECT_TILT_WARNING       28
#define SOUND_EFFECT_MATCH_SPIN         30
#define SOUND_EFFECT_LEFT_SPINNER       32
#define SOUND_EFFECT_RIGHT_SPINNER      33
#define SOUND_EFFECT_SLING_SHOT         34
//#define SOUND_EFFECT_10PT_SWITCH        36
#define SOUND_EFFECT_BULLSEYE_UNLIT     37
#define SOUND_EFFECT_BULLSEYE_LIT       38
#define SOUND_EFFECT_SR_FINISHED   40
#define SOUND_EFFECT_WS_FINISHED      41
#define SOUND_EFFECT_SRWS_FINISHED    42
#define SOUND_EFFECT_CAPTIVE_BALL_UNLIT       43
#define SOUND_EFFECT_ENEMY_INVASION_ALARM     44
#define SOUND_EFFECT_INVADING_ENEMY_HIT       45
#define SOUND_EFFECT_INVADING_ENEMY_MISS      46
#define SOUND_EFFECT_SHIELD_DESTROYED         47
#define SOUND_EFFECT_NEUTRAL_ZONE_DUPLICATE   48
#define SOUND_EFFECT_NEUTRAL_ZONE_HIT         49
//#define SOUND_EFFECT_SHOOT_AGAIN        60
#define SOUND_EFFECT_TILT               61
#define SOUND_EFFECT_BATTLE_START       62
#define SOUND_EFFECT_BATTLE_ADD_ENEMY   63
#define SOUND_EFFECT_BATTLE_LOST        64
#define SOUND_EFFECT_BATTLE_WON         65
#define SOUND_EFFECT_BATTLE_ENEMY_HIT   66
#define SOUND_EFFECT_SCORE_TICK         67
//#define SOUND_EFFECT_VOICE_EXTRA_BALL   81
#define SOUND_EFFECT_WIZARD_START       82
//#define SOUND_EFFECT_WIZARD_FINISHED    83

#define SOUND_EFFECT_WAV_MANDATORY      100 
#define SOUND_EFFECT_COIN_DROP_1        100
#define SOUND_EFFECT_COIN_DROP_2        101
#define SOUND_EFFECT_COIN_DROP_3        102
#define SOUND_EFFECT_WIZARD_START_SAUCER  110
#define SOUND_EFFECT_WIZARD_FINAL_SHOT_1  111
#define SOUND_EFFECT_WIZARD_FINAL_SHOT_2  112
#define SOUND_EFFECT_MACHINE_START      120

#define SOUND_EFFECT_SELF_TEST_MODE_START             132
#define SOUND_EFFECT_SELF_TEST_CPC_START              180
#define SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START    190

#define SOUND_EFFECT_BACKGROUND_SONG_1    200
#define NUM_BACKGROUND_SONGS              9
#define SOUND_EFFECT_BATTLE_SONG_1        250
#define NUM_BATTLE_SONGS                  3
#define SOUND_EFFECT_BACKGROUND_SONG_WIZARD            275
#define SOUND_EFFECT_BACKGROUND_SONG_WIZARD_LAST_10    276

unsigned long BackgroundSongEndTime;

// Game play status callouts
#define NUM_VOICE_NOTIFICATIONS                 104
byte VoiceNotificationDurations[NUM_VOICE_NOTIFICATIONS] = {
  4, 3, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 3, 2, 4, 3, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 4, 4, 4, 4, 4, 4, 4, 4, 5,
  4, 5, 5, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 2, 3, 2, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 4, 3, 3, 3, 3, 3,
  3, 4, 4, 4, 4, 3, 3, 3, 3, 4,
  3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
  4, 4, 5, 5, 4, 5, 3, 3, 3, 4,
  4, 6, 4, 6
};
unsigned long NextVoiceNotificationPlayTime;

#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START                 500
#define SOUND_EFFECT_VP_INTERCEPTING_ENEMY_TRANSMISSION           500
#define SOUND_EFFECT_VP_INTRUDER_ALERT                            501
#define SOUND_EFFECT_VP_CHICKEN                                   502
#define SOUND_EFFECT_VP_INCOMING_ENEMY_VECTOR_S1                  503
#define SOUND_EFFECT_VP_INCOMING_ENEMY_VECTOR_S2                  513
#define SOUND_EFFECT_VP_BATTLE_WON                                514
#define SOUND_EFFECT_VP_BATTLE_LOST                               515
#define SOUND_EFFECT_VP_PRISONER_WAITING                          516
#define SOUND_EFFECT_VP_SMART_BOMB                                517
#define SOUND_EFFECT_VP_SR                                   518
#define SOUND_EFFECT_VP_WS                                      519
#define SOUND_EFFECT_VP_SHIELD_COMPLETE                           520
#define SOUND_EFFECT_VP_SCANNER_CHARGED                           521
#define SOUND_EFFECT_VP_BONUSX_INCREASED                          522
#define SOUND_EFFECT_VP_BONUSX_MAX                                523
#define SOUND_EFFECT_VP_BONUS_MAX                                 524
#define SOUND_EFFECT_VP_COMBO_1                                   525
#define SOUND_EFFECT_VP_COMBO_2                                   526
#define SOUND_EFFECT_VP_COMBO_3                                   527
#define SOUND_EFFECT_VP_COMBO_4                                   528
#define SOUND_EFFECT_VP_COMBO_5                                   529
#define SOUND_EFFECT_VP_COMBO_6                                   530
#define SOUND_EFFECT_VP_COMBOS_COMPLETE                           531
#define SOUND_EFFECT_VP_SEVEN_NEUTRAL_ZONES                       532
#define SOUND_EFFECT_VP_CYCLING_WS                              533
#define SOUND_EFFECT_VP_HUMAN_POSITION_1                          534
#define SOUND_EFFECT_VP_HUMAN_POSITION_4                          537
#define SOUND_EFFECT_VP_1X_PLAYFIELD                              538
#define SOUND_EFFECT_VP_5X_PLAYFIELD                              542

#define SOUND_EFFECT_VP_ADD_PLAYER_1        543
#define SOUND_EFFECT_VP_ADD_PLAYER_2        (SOUND_EFFECT_VP_ADD_PLAYER_1+1)
#define SOUND_EFFECT_VP_ADD_PLAYER_3        (SOUND_EFFECT_VP_ADD_PLAYER_1+2)
#define SOUND_EFFECT_VP_ADD_PLAYER_4        (SOUND_EFFECT_VP_ADD_PLAYER_1+3)
#define SOUND_EFFECT_VP_PLAYER_1_UP         547
#define SOUND_EFFECT_VP_PLAYER_2_UP         (SOUND_EFFECT_VP_PLAYER_1_UP+1)
#define SOUND_EFFECT_VP_PLAYER_3_UP         (SOUND_EFFECT_VP_PLAYER_1_UP+2)
#define SOUND_EFFECT_VP_PLAYER_4_UP         (SOUND_EFFECT_VP_PLAYER_1_UP+3)
#define SOUND_EFFECT_VP_SHOOT_AGAIN         551

#define SOUND_EFFECT_VP_JACKPOT                                 552
#define SOUND_EFFECT_VP_SUPER_JACKPOT                           553
#define SOUND_EFFECT_VP_EXTRA_BALL                              554

#define SOUND_EFFECT_VP_LAUNCH_PROMPT_1_1         555
#define SOUND_EFFECT_VP_LAUNCH_PROMPT_2_1         559
#define SOUND_EFFECT_VP_LAUNCH_PROMPT_3_1         563
#define SOUND_EFFECT_VP_ENEMY_ADVANCING           567
#define SOUND_EFFECT_VP_ENEMY_INVASION_DEFEATED   568
#define SOUND_EFFECT_VP_ENEMY_DESTROYED_SHIELD    569
#define SOUND_EFFECT_VP_SPINNER_GOAL_REACHED      570
#define SOUND_EFFECT_VP_BASES_GOAL_REACHED        571
#define SOUND_EFFECT_VP_ADVERTISE_BATTLE          572
#define SOUND_EFFECT_VP_ADVERTISE_INVASION        573
#define SOUND_EFFECT_VP_ADVERTISE_COMBOS          574
#define SOUND_EFFECT_VP_ADVERTISE_SPINS           575
#define SOUND_EFFECT_VP_ADVERTISE_NZS             576
#define SOUND_EFFECT_VP_ADVERTISE_SHIELD          577
#define SOUND_EFFECT_VP_ADVERTISE_BASES           578
#define SOUND_EFFECT_VP_FIVE_COMBOS_LEFT          579
#define SOUND_EFFECT_VP_SEVEN_GOALS_FOR_ENEMY     584
#define SOUND_EFFECT_VP_ONE_GOAL_FOR_ENEMY        590
#define SOUND_EFFECT_VP_ALL_GOALS_DONE            591
#define SOUND_EFFECT_VP_WIZARD_20_SECONDS         592
#define SOUND_EFFECT_VP_WIZARD_30_SECONDS         593
#define SOUND_EFFECT_VP_WIZARD_45_SECONDS         594
#define SOUND_EFFECT_VP_WIZARD_60_SECONDS         595
#define SOUND_EFFECT_VP_WIZARD_10_SECONDS_LEFT    596
#define SOUND_EFFECT_VP_WIZARD_100_PERCENT_HIT    597
#define SOUND_EFFECT_VP_WIZARD_50_PERCENT_HIT     598
#define SOUND_EFFECT_VP_WIZARD_10_PERCENT_HIT     599
#define SOUND_EFFECT_VP_ORBIT_ABANDONED           600
#define SOUND_EFFECT_VP_SAUCER_TO_ORBIT           601
#define SOUND_EFFECT_VP_PREPARING_MISSLES         602
#define SOUND_EFFECT_VP_WIZARD_END_COLLECT        603


#define MAX_DISPLAY_BONUS     39
#define TILT_WARNING_DEBOUNCE_TIME      1000


/*********************************************************************

    Machine state and options

*********************************************************************/
byte Credits = 0;
byte SoundSelector = 3;
byte BallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte ScoreAwardReplay = 0;
byte MusicVolume = 6;
byte SoundEffectsVolume = 8;
byte CalloutsVolume = 10;
byte ChuteCoinsInProgress[3];
boolean FreePlayMode = false;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long HighScore = 0;
unsigned long AwardScores[3];


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus[4];
byte CurrentBonus;
byte BonusX[4];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean ExtraBallCollected = false;
boolean SpecialCollected = false;
boolean TimersPaused = true;
boolean AllowResetAfterBallOne = true;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown;
unsigned long PlayfieldMultiplier;
unsigned long LastTimeThroughLoop;
unsigned long LastSwitchHitTime;
unsigned long BallSaveEndTime;

#define BALL_SAVE_GRACE_PERIOD  3000

/*********************************************************************

    Game Specific State Variables

*********************************************************************/
int NumCarryWizardGoals[4];
byte TotalSpins[4];
byte SpinnerMaxGoal = 250;
byte LastAwardShotCalloutPlayed;
byte LastWizardTimer;
byte TopLaneStatus[4];
byte TopLaneSkillShot;
byte LeftDropTargetStatus;
byte CenterDropTargetStatus;
byte RightDropTargetStatus;
byte SkillShotLane;
byte NumLeftDTClears[4];
byte NumCenterDTClears[4];
byte NumRightDTClears[4];
byte LowerPopStatus[4];
byte CombosAchieved[4];
byte HoldoverAwards[4];
byte SWLettersLevel[4];
byte NeutralZoneHits[4];
byte SaucerValue[4];
byte WizardGoals[4];
byte UsedWizardGoals[4];
byte BasesVisited;
byte IdleMode;
byte CombosToFinishGoal = 6;
#define NUM_BALL_SEARCH_SOLENOIDS   8
byte BallSearchSolenoidToTry;
byte BallSearchSols[NUM_BALL_SEARCH_SOLENOIDS] = {SOL_BOTTOM_LEFT_POP, SOL_SAUCER, SOL_UPPER_LEFT_POP, SOL_UPPER_RIGHT_POP, SOL_CENTER_POP, SOL_BOTTOM_RIGHT_POP, SOL_RIGHT_SLING, SOL_LEFT_SLING};
byte GoalsUntilWizard;
byte WizardModeTime;

boolean IdleModeOn = true;
boolean SpinnerAccelerators = true;
boolean OutlaneSpecialLit[4];
boolean CaptiveBallLit[4];
boolean BullseyeSpecialLit[4];


unsigned long LastInlaneHitTime;
unsigned long BonusXAnimationStart;
unsigned long LastSpinnerHit;
unsigned long ResetLeftDropTargetStatusTime;
unsigned long ResetCenterDropTargetStatusTime;
unsigned long ResetRightDropTargetStatusTime;

unsigned long TopCenterPopLastHit;
unsigned long TopLeftPopLastHit;
unsigned long TopRightPopLastHit;
unsigned long TopLaneAnimationStartTime[4];
unsigned long SWLettersAnimationStartTime[11];
unsigned long SaucerScoreAnimationStart;
unsigned long BattleAward;
unsigned long SaucerEjectTime;
unsigned long PlayfieldMultiplierExpiration;
unsigned long UpperPopFrenzyFinish;
unsigned long SpinnerFrenzyEndTime;
unsigned long ShieldDestroyedAnimationStart;
unsigned long TicksCountedTowardsInvasion;
unsigned long TicksCountedTowardsStatus;
unsigned long SWLettersExpirationTime[11];
unsigned long BallSearchNextSolenoidTime;
unsigned long BallSearchSolenoidFireTime[NUM_BALL_SEARCH_SOLENOIDS];
unsigned long WizardBonus;
unsigned long LastWizardBonus;
unsigned long LastTimeWizardBonusShown;
unsigned long TimeInSaucer;

// Combo tracking variables
unsigned long LastLeftInlane;
unsigned long LastRightInlane;

unsigned short SWStatus[4];
unsigned short BattleLetter;
unsigned short InvasionPosition;
unsigned short InvasionFlashLevel;

#define INVASION_POSITION_NONE            0x0000
#define INVASION_POSITION_TOP_1           0x0001
#define INVASION_POSITION_TOP_2           0x0002
#define INVASION_POSITION_TOP_3           0x0004
#define INVASION_POSITION_TOP_4           0x0008
#define INVASION_POSITION_TOP_LANE_MASK   0x000F
#define INVASION_POSITION_TL_POP          0x0010
#define INVASION_POSITION_TR_POP          0x0020
#define INVASION_POSITION_MIDDLE_POP      0x0040
#define INVASION_POSITION_CAPTIVE         0x0080
#define INVASION_POSITION_BULLSEYE        0x0100
#define INVASION_POSITION_LOWER_POPS      0x0200
#define INVASION_START_WAIT_TIME          20000

#define COMBO_AVAILABLE_TIME              3500
#define COMBO_LEFT_TO_RIGHT_ALLEY_PASS    0
#define COMBO_RIGHT_TO_LEFT_ALLEY_PASS    1
#define COMBO_LEFT_TO_RIGHT_SPINNER       2
#define COMBO_RIGHT_TO_LEFT_SPINNER       3
#define COMBO_LEFT_TO_BULLSEYE            4
#define COMBO_RIGHT_TO_CAPTIVE            5
#define COMBO_AWARD               10000
#define COMBOS_COMPLETE_AWARD     75000

#define SW_LETTER_S1_INDEX  0
#define SW_LETTER_T_INDEX   1
#define SW_LETTER_E_INDEX   2
#define SW_LETTER_L1_INDEX  3
#define SW_LETTER_L2_INDEX  4
#define SW_LETTER_A1_INDEX  5
#define SW_LETTER_R1_INDEX  6
#define SW_LETTER_W_INDEX   7
#define SW_LETTER_A2_INDEX  8
#define SW_LETTER_R2_INDEX  9
#define SW_LETTER_S2_INDEX  10
#define SW_STATUS_WS_MASK 0x0780

#define SW_LETTERS_SHIELD_COMPLETE_TIME   30000
#define SW_LETTERS_EXPIRATION_BASE        60000
#define SRWS_COMPLETION_BONUS     50000

#define WIZARD_MODE_QUALIFY_TICKS         45000
#define WIZARD_START_DURATION             5000
#define WIZARD_DURATION                   39000
#define WIZARD_DURATION_SECONDS           39
#define WIZARD_FINISHED_DURATION          5000
#define WIZARD_SWITCH_SCORE               5000
#define WIZARD_MODE_REWARD_SCORE          250000

#define HOLDOVER_BONUS_X      0x01
#define HOLDOVER_BONUS        0x02

#define NEUTRAL_ZONE_1        0
#define NEUTRAL_ZONE_2        1
#define NEUTRAL_ZONE_3        2
#define NEUTRAL_ZONE_4        3
#define NEUTRAL_ZONE_5        4
#define NEUTRAL_ZONE_6        5
#define NEUTRAL_ZONE_7        6

#define WIZARD_GOAL_7_NZ      0x01
#define WIZARD_GOAL_SPINS     0x02
#define WIZARD_GOAL_BATTLE    0x04
#define WIZARD_GOAL_INVASION  0x08
#define WIZARD_GOAL_SHIELD    0x10
#define WIZARD_GOAL_COMBOS    0x20
#define WIZARD_GOAL_POP_BASES 0x40

#define BASE_VISIT_BOTTOM_LEFT_POP    0x01
#define BASE_VISIT_BOTTOM_RIGHT_POP   0x02
#define BASE_VISIT_TOP_CENTER_POP     0x04
#define BASE_VISIT_TOP_LEFT_POP       0x08
#define BASE_VISIT_TOP_RIGHT_POP      0x10
#define BASES_ALL_VISITED             0x1F

#define IDLE_MODE_NONE                  0
#define IDLE_MODE_ANNOUNCE_GOALS        1
#define IDLE_MODE_ADVERTISE_BATTLE      2
#define IDLE_MODE_ADVERTISE_INVASION    3
#define IDLE_MODE_ADVERTISE_COMBOS      4
#define IDLE_MODE_ADVERTISE_BASES       5
#define IDLE_MODE_ADVERTISE_NZS         6
#define IDLE_MODE_ADVERTISE_SPINS       7
#define IDLE_MODE_ADVERTISE_SHIELD      8
#define IDLE_MODE_BALL_SEARCH           9

#define SAUCER_VALUE_1K   0
#define SAUCER_VALUE_2K   1
#define SAUCER_VALUE_5K   2
#define SAUCER_VALUE_10K  3
#define SAUCER_VALUE_EB   4


void ReadStoredParameters() {
   for (byte count=0; count<3; count++) {
    ChuteCoinsInProgress[count] = 0;
  }
 
  HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;

  SoundSelector = ReadSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
  if (SoundSelector > 8) SoundSelector = 3;

  MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
  if (MusicVolume>10) MusicVolume = 10;

  SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10);
  if (SoundEffectsVolume>10) SoundEffectsVolume = 10;

  CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
  if (CalloutsVolume>10) CalloutsVolume = 10;

  GoalsUntilWizard = ReadSetting(EEPROM_GOALS_UNTIL_WIZ_BYTE, 5);
  if (GoalsUntilWizard>7) GoalsUntilWizard = 7;

  CombosToFinishGoal = ReadSetting(EEPROM_COMBOS_GOAL_BYTE, 6);
  if (CombosToFinishGoal>6) CombosToFinishGoal = 6;

  IdleModeOn = (ReadSetting(EEPROM_IDLE_MODE_BYTE, 1)!=0) ? true:false;

  WizardModeTime = ReadSetting(EEPROM_WIZ_TIME_BYTE, 30);
  if (WizardModeTime>60) WizardModeTime = 30;

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  AllowResetAfterBallOne = (ReadSetting(EEPROM_ALLOW_RESET_BYTE, 1)) ? true : false;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;

  ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_UL);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_UL);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);

}


void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }

  // Set up the chips and interrupts
  RPU_InitializeMPU(SW_CREDIT_RESET);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();
  RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  CurrentScores[0] = SPACE_BATTLE_MAJOR_VERSION;
  CurrentScores[1] = SPACE_BATTLE_MINOR_VERSION;
  CurrentScores[2] = RPU_OS_MAJOR_VERSION;
  CurrentScores[3] = RPU_OS_MINOR_VERSION;

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  // WAV Trigger startup at 57600
  wTrig.start();
  wTrig.stopAllTracks();
  delayMicroseconds(10000);
#endif
  InitSoundEffectQueue();
  StopAudio();
  CurrentTime = millis();
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);
}

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}


// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (RPU_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}


////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////
void ShowLampAnimation(byte animationNum, unsigned long divisor, unsigned long baseTime, byte subOffset, boolean dim, boolean reverse = false, byte keepLampOn = 99) {
  byte currentStep = (baseTime / divisor) % LAMP_ANIMATION_STEPS;
  if (reverse) currentStep = (LAMP_ANIMATION_STEPS - 1) - currentStep;

  byte lampNum = 0;
  for (int byteNum = 0; byteNum < 8; byteNum++) {
    byte bitMask = 0x01;
    for (byte bitNum = 0; bitNum < 8; bitNum++) {

      // if there's a subOffset, turn off lights at that offset
      if (subOffset) {
        byte lampOff = true;
        lampOff = LampAnimations[animationNum][(currentStep + subOffset) % LAMP_ANIMATION_STEPS][byteNum] & bitMask;
        if (lampOff && lampNum != keepLampOn) RPU_SetLampState(lampNum, 0);
      }

      byte lampOn = false;
      lampOn = LampAnimations[animationNum][currentStep][byteNum] & bitMask;
      if (lampOn) RPU_SetLampState(lampNum, 1, dim);

      lampNum += 1;
      bitMask *= 2;
    }
  }
}

unsigned long ToplaneDeltaTicks = 0;
unsigned long ToplaneLastTicks = 0;
unsigned long ToplaneCycle = 0;

void ShowTopLaneLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_1 + count, count == SkillShotLane, 0, 250);
    }
  } else if (IdleMode != IDLE_MODE_NONE) {
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_1 + count, IdleMode == IDLE_MODE_ADVERTISE_INVASION, 0, 250);
    }
  } else if (InvasionPosition & INVASION_POSITION_TOP_LANE_MASK) {
    unsigned short laneMask = 0x0001;
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_1 + count, (InvasionPosition & laneMask) ? true : false, 0, InvasionFlashLevel);
      laneMask *= 2;
    }
  } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY && TicksCountedTowardsInvasion != 0) {
    unsigned long period = 50 + ((INVASION_START_WAIT_TIME - TicksCountedTowardsInvasion) / 150);

    ToplaneDeltaTicks += (CurrentTime - ToplaneLastTicks);
    if (ToplaneDeltaTicks > period) {
      ToplaneCycle += ToplaneDeltaTicks / period;
      ToplaneDeltaTicks = ToplaneDeltaTicks % period;
    }
    ToplaneLastTicks = CurrentTime;

    byte lampPhase = ToplaneCycle % 24;
    byte lampNum = lampPhase / 4;
    if (lampNum > 3) lampNum = 6 - lampNum;
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_1 + count, ((lampPhase % 4) == 0) && (count == lampNum));
    }
  } else {
    byte bitMask = 0x01;
    for (byte count = 0; count < 4; count++) {
      if (TopLaneAnimationStartTime[count] != 0) {
        RPU_SetLampState(LAMP_1 + count, 1, 0, 100);
        if ((CurrentTime - TopLaneAnimationStartTime[count]) > 5000) TopLaneAnimationStartTime[count] = 0;
      } else {
        byte laneOn = (TopLaneStatus[CurrentPlayer] & bitMask);
        RPU_SetLampState(LAMP_1 + count, laneOn);
      }
      bitMask *= 2;
    }
  }
}


void ShowBonusLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime - GameModeStartTime) / 50) % 25;
    for (byte count = 0; count < 9; count++) {
      RPU_SetLampState(LAMP_BONUS_1 + count, count >= (22 - lampPhase) && count <= (24 - lampPhase));
    }
    RPU_SetLampState(LAMP_BONUS_10, 0);
    RPU_SetLampState(LAMP_BONUS_20, 0);
  } else {
    RPU_SetLampState(LAMP_BONUS_10, ((CurrentBonus / 10) % 2) == 1);
    RPU_SetLampState(LAMP_BONUS_20, (CurrentBonus / 10) > 1);
    for (byte count = 0; count < 9; count++) {
      RPU_SetLampState(LAMP_BONUS_1 + count, count < (CurrentBonus % 10));
    }
  }
}


//          0  1  2  3  4  5  6     0  1  2  3
//          S  T  E  L  L  A  R     W  A  R  S
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
//          29 28 27 26 25 24 23 22 21 20 19 18 17

unsigned short lastSWStatusReported = 0;

void ShowSRWSCircleLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime - GameModeStartTime) / 50) % 25;
    for (byte count = 0; count < 7; count++) {
      if (lampPhase < 23) RPU_SetLampState(LAMP_CIRCLE_S1 + count, count >= lampPhase && count <= (lampPhase + 2));
      else RPU_SetLampState(LAMP_CIRCLE_S1 + count, count <= (lampPhase - 23));
    }
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_CIRCLE_W + count, count >= (lampPhase - 8) && count <= (lampPhase - 6));
    }
  } else if (IdleMode == IDLE_MODE_ADVERTISE_SHIELD) {
    for (byte count = 0; count < 7; count++) {
      RPU_SetLampState(LAMP_CIRCLE_S1 + count, 1, 0, 250);
    }
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_CIRCLE_W + count, 1, 0, 250);
    }
  } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE_START || (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE_ADD_ENEMY) {

    int lampPhase = ((CurrentTime - GameModeStartTime) / 75) % 30;
    unsigned short bitMask = 0x0001;
    for (int count = 0; count < 7; count++) {
      boolean battleLetterOn = (BattleLetter & bitMask) ? true : false;
      if (battleLetterOn) {
        if (lampPhase < 15) {
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, (lampPhase + 2) >= (count + 3) && (lampPhase - 2) <= (count + 3), 0, 25);
        } else {
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, (27 - lampPhase) <= (count) && (31 - lampPhase) >= (count), 0, 25);
        }
      } else {
        if (lampPhase < 15) {
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, (lampPhase + 2) >= (count + 3) && (lampPhase) <= (count + 3));
        } else {
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, (27 - lampPhase) <= (count) && (29 - lampPhase) >= (count));
        }
      }
      bitMask *= 2;
    }

    bitMask = 0x0080;
    for (int count = 0; count < 4; count++) {
      boolean battleLetterOn = (BattleLetter & bitMask) ? true : false;
      if (battleLetterOn) {
        if (lampPhase < 15) {
          RPU_SetLampState(LAMP_CIRCLE_W + count, (lampPhase + 2) >= (count + 11) && (lampPhase - 2) <= (count + 11), 0, 25);
        } else {
          RPU_SetLampState(LAMP_CIRCLE_W + count, (19 - lampPhase) <= (count) && (23 - lampPhase) >= (count), 0, 25);
        }
      } else {
        if (lampPhase < 15) {
          RPU_SetLampState(LAMP_CIRCLE_W + count, (lampPhase + 2) >= (count + 11) && (lampPhase) <= (count + 11));
        } else {
          RPU_SetLampState(LAMP_CIRCLE_W + count, (19 - lampPhase) <= (count) && (21 - lampPhase) >= (count));
        }
      }
      bitMask *= 2;
    }
  } else if (ShieldDestroyedAnimationStart) {

    byte letterPhase = ((CurrentTime - ShieldDestroyedAnimationStart) / 500) % 7;
    for (int count = 0; count < 12; count++) {
      if (count == 7) continue;
      if (count < 7) RPU_SetLampState(LAMP_CIRCLE_S1 + count, (count < (6 - letterPhase)) || (count > (5 + letterPhase)) );
      else RPU_SetLampState(LAMP_CIRCLE_S1 + (count - 1), (count < (6 - letterPhase)) || (count > (5 + letterPhase)) );
    }

    if (CurrentTime > (ShieldDestroyedAnimationStart + 4000)) ShieldDestroyedAnimationStart = 0;
  } else {
    unsigned short bitMask = 0x0001;
    for (int count = 0; count < 11; count++) {
      if ((BattleLetter & bitMask)) {
        RPU_SetLampState(LAMP_CIRCLE_S1 + count, 1, 0, 25);
      } else {
        if (SWLettersAnimationStartTime[count] != 0) {
          int flashPeriod = 300;
          if ((CurrentTime - SWLettersAnimationStartTime[count]) > 3000) flashPeriod = 100;
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, 1, 0, flashPeriod);
          if ((CurrentTime - SWLettersAnimationStartTime[count]) > 5000) SWLettersAnimationStartTime[count] = 0;
        } else if (SWLettersExpirationTime[count] != 0 && (CurrentTime + 10000) > SWLettersExpirationTime[count]) {
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, (SWStatus[CurrentPlayer]&bitMask) ? true : false, 0, 175);
        } else {
          RPU_SetLampState(LAMP_CIRCLE_S1 + count, (SWStatus[CurrentPlayer]&bitMask) ? true : false);
        }
      }
      bitMask *= 2;
    }
  }
}

boolean BonusXLamp2[] = {0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1};
boolean BonusXLamp3[] = {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1};
boolean BonusXLamp4[] = {0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1};
boolean BonusXLamp5[] = {0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1};

void ShowBonusXLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    RPU_SetLampState(LAMP_BONUS_2X, BonusXLamp2[BonusX[CurrentPlayer]]);
    RPU_SetLampState(LAMP_BONUS_3X, BonusXLamp3[BonusX[CurrentPlayer]]);
    RPU_SetLampState(LAMP_BONUS_4X, BonusXLamp4[BonusX[CurrentPlayer]]);
    RPU_SetLampState(LAMP_BONUS_5X, BonusXLamp5[BonusX[CurrentPlayer]]);
  } else {
    int flashSpeed = 0;
    if (BonusXAnimationStart != 0) flashSpeed = 200;
    if ((CurrentTime - BonusXAnimationStart) > 3000) BonusXAnimationStart = 0;
    RPU_SetLampState(LAMP_BONUS_2X, BonusXLamp2[BonusX[CurrentPlayer]], 0, flashSpeed);
    RPU_SetLampState(LAMP_BONUS_3X, BonusXLamp3[BonusX[CurrentPlayer]], 0, flashSpeed);
    RPU_SetLampState(LAMP_BONUS_4X, BonusXLamp4[BonusX[CurrentPlayer]], 0, flashSpeed);
    RPU_SetLampState(LAMP_BONUS_5X, BonusXLamp5[BonusX[CurrentPlayer]], 0, flashSpeed);
  }
}


void ShowLaneRolloverAndTargetLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    RPU_SetLampState(LAMP_W_ROLLOVER, 0);
    RPU_SetLampState(LAMP_A_ROLLOVER, 0);
    RPU_SetLampState(LAMP_R_ROLLOVER, 0);
    RPU_SetLampState(LAMP_S_ROLLOVER, 0);
    RPU_SetLampState(LAMP_CAPTIVE_BALL, 0);
    RPU_SetLampState(LAMP_OUTLANE_SPECIAL, 0);
    RPU_SetLampState(LAMP_BULLSEYE_SPECIAL, 0);
  } else if (IdleMode != IDLE_MODE_NONE) {
    RPU_SetLampState(LAMP_W_ROLLOVER, IdleMode == IDLE_MODE_ADVERTISE_COMBOS, 0, 250);
    RPU_SetLampState(LAMP_A_ROLLOVER, IdleMode == IDLE_MODE_ADVERTISE_COMBOS, 0, 250);
    RPU_SetLampState(LAMP_R_ROLLOVER, IdleMode == IDLE_MODE_ADVERTISE_COMBOS, 0, 250);
    RPU_SetLampState(LAMP_S_ROLLOVER, IdleMode == IDLE_MODE_ADVERTISE_COMBOS, 0, 250);
    RPU_SetLampState(LAMP_CAPTIVE_BALL, IdleMode == IDLE_MODE_ADVERTISE_COMBOS, 0, 250);
    RPU_SetLampState(LAMP_OUTLANE_SPECIAL, 0);
    RPU_SetLampState(LAMP_BULLSEYE_SPECIAL, IdleMode == IDLE_MODE_ADVERTISE_COMBOS, 0, 250);
  } else {
    boolean lampWHandled = false;
    boolean lampSHandled = false;
    byte lampPhase = 255;
    lampPhase = ((CurrentTime - LastLeftInlane) / 140) % 3;

    if (InvasionPosition != INVASION_POSITION_NONE) {
      RPU_SetLampState(LAMP_BULLSEYE_SPECIAL, (InvasionPosition & INVASION_POSITION_BULLSEYE) ? true : false, 0, InvasionFlashLevel);
      RPU_SetLampState(LAMP_CAPTIVE_BALL, (InvasionPosition & INVASION_POSITION_CAPTIVE) ? true : false, 0, InvasionFlashLevel);
    } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD) {
      RPU_SetLampState(LAMP_BULLSEYE_SPECIAL, 1, 0, 100);
      RPU_SetLampState(LAMP_CAPTIVE_BALL, 1, 0, 100);
    } else {
      if (LastLeftInlane && CurrentTime < (LastLeftInlane + COMBO_AVAILABLE_TIME)) {
        RPU_SetLampState(LAMP_BULLSEYE_SPECIAL, lampPhase == 1 && !(CombosAchieved[CurrentPlayer] & (1 << COMBO_LEFT_TO_BULLSEYE)));
      } else {
        RPU_SetLampState(LAMP_BULLSEYE_SPECIAL, BullseyeSpecialLit[CurrentPlayer]);
      }

      if (LastRightInlane && CurrentTime < (LastRightInlane + COMBO_AVAILABLE_TIME)) {
        RPU_SetLampState(LAMP_CAPTIVE_BALL, lampPhase == 1 && !(CombosAchieved[CurrentPlayer] & (1 << COMBO_RIGHT_TO_CAPTIVE)));
      } else {
        RPU_SetLampState(LAMP_CAPTIVE_BALL, CaptiveBallLit[CurrentPlayer]);
      }
    }


    if (LastRightInlane && CurrentTime < (LastRightInlane + COMBO_AVAILABLE_TIME)) {
      if ( (CombosAchieved[CurrentPlayer] & (1 << COMBO_RIGHT_TO_LEFT_ALLEY_PASS)) == 0 ) {
        RPU_SetLampState(LAMP_W_ROLLOVER, lampPhase == 2);
        lampWHandled = true;
      }
    }

    if (LastLeftInlane && CurrentTime < (LastLeftInlane + COMBO_AVAILABLE_TIME)) {
      if ( (CombosAchieved[CurrentPlayer] & (1 << COMBO_LEFT_TO_RIGHT_ALLEY_PASS)) == 0 ) {
        RPU_SetLampState(LAMP_S_ROLLOVER, lampPhase == 2);
        lampSHandled = true;
      }
    }


    unsigned short bitMask = 0x0080;
    for (int count = 0; count < 4; count++) {
      if (count == 0 && lampWHandled) {
        bitMask *= 2;
        continue;
      }
      if (count == 3 && lampSHandled) {
        bitMask *= 2;
        continue;
      }

      boolean lampOn = (((SWStatus[CurrentPlayer])&bitMask) != 0) ? false : true;
      int flashValue = 0;
      if (BattleLetter & bitMask) {
        flashValue = 50;
        lampOn = true;
      }
      RPU_SetLampState(LAMP_W_ROLLOVER + count, lampOn, 0, flashValue);
      bitMask *= 2;
    }

    RPU_SetLampState(LAMP_OUTLANE_SPECIAL, OutlaneSpecialLit[CurrentPlayer]);

  }
}


void ShowSpinnerLamps() {
  if ( (GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    RPU_SetLampState(LAMP_SPINNERS, 0);
  } else if (IdleMode != IDLE_MODE_NONE) {
    RPU_SetLampState(LAMP_SPINNERS, (IdleMode == IDLE_MODE_ADVERTISE_SPINS) || (IdleMode == IDLE_MODE_ADVERTISE_COMBOS), 0, 250);
  } else {
    if (LastLeftInlane && CurrentTime < (LastLeftInlane + COMBO_AVAILABLE_TIME)) {
      byte lampPhase = ((CurrentTime - LastLeftInlane) / 140) % 3;
      RPU_SetLampState(LAMP_SPINNERS, lampPhase == 0 && !(CombosAchieved[CurrentPlayer] & (1 << COMBO_LEFT_TO_RIGHT_SPINNER)));
    } else if (LastRightInlane && CurrentTime < (LastRightInlane + COMBO_AVAILABLE_TIME)) {
      byte lampPhase = ((CurrentTime - LastRightInlane) / 140) % 3;
      RPU_SetLampState(LAMP_SPINNERS, lampPhase == 0 && !(CombosAchieved[CurrentPlayer] & (1 << COMBO_RIGHT_TO_LEFT_SPINNER)));
    } else if (SpinnerFrenzyEndTime) {
      int flash = 250;
      if ( (CurrentTime + 2000) > SpinnerFrenzyEndTime ) flash = 150;
      RPU_SetLampState(LAMP_SPINNERS, 1, 0, flash);
    } else if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS) {
      RPU_SetLampState(LAMP_SPINNERS, 1);
    } else {
      RPU_SetLampState(LAMP_SPINNERS, 0);
    }

  }
}


void ShowShootAgainLamps() {

  if (!BallSaveUsed && (BallSaveEndTime||BallSaveNumSeconds) && (CurrentTime<BallSaveEndTime) ) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime!=0) msRemaining = BallSaveEndTime - CurrentTime;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
    RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
  } else if ( (GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD ) {
    RPU_SetLampState(LAMP_SHOOT_AGAIN, 1);
    RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, 1);
  } else {
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, SamePlayerShootsAgain);
  }
}


void ShowPopBumperLamps() {
  if (InvasionPosition != INVASION_POSITION_NONE) {
    RPU_SetLampState(LAMP_TOP_LEFT_POP_BUMPER, (InvasionPosition & INVASION_POSITION_TL_POP) ? true : false, 0, InvasionFlashLevel);
    RPU_SetLampState(LAMP_TOP_CENTER_POP_BUMPER, (InvasionPosition & INVASION_POSITION_MIDDLE_POP) ? true : false, 0, InvasionFlashLevel);
    RPU_SetLampState(LAMP_TOP_RIGHT_POP_BUMPER, (InvasionPosition & INVASION_POSITION_TR_POP) ? true : false, 0, InvasionFlashLevel);
    RPU_SetLampState(LAMP_BOTTOM_POP_BUMPERS, (InvasionPosition & INVASION_POSITION_LOWER_POPS) ? true : false, 0, InvasionFlashLevel);
  } else if (IdleMode != IDLE_MODE_NONE) {
    RPU_SetLampState(LAMP_TOP_LEFT_POP_BUMPER, (IdleMode == IDLE_MODE_ADVERTISE_BASES) ? true : false, 0, 250);
    RPU_SetLampState(LAMP_TOP_CENTER_POP_BUMPER, (IdleMode == IDLE_MODE_ADVERTISE_BASES) ? true : false, 0, 250);
    RPU_SetLampState(LAMP_TOP_RIGHT_POP_BUMPER, (IdleMode == IDLE_MODE_ADVERTISE_BASES) ? true : false, 0, 250);
    RPU_SetLampState(LAMP_BOTTOM_POP_BUMPERS, (IdleMode == IDLE_MODE_ADVERTISE_BASES) ? true : false, 0, 250);
  } else {
    if (UpperPopFrenzyFinish != 0) {
      byte popPhase = (CurrentTime / 250) % 3;
      RPU_SetLampState(LAMP_TOP_LEFT_POP_BUMPER, popPhase == 0);
      RPU_SetLampState(LAMP_TOP_CENTER_POP_BUMPER, popPhase == 1);
      RPU_SetLampState(LAMP_TOP_RIGHT_POP_BUMPER, popPhase == 2);
      if (CurrentTime > UpperPopFrenzyFinish) UpperPopFrenzyFinish = 0;
    } else {
      RPU_SetLampState(LAMP_TOP_LEFT_POP_BUMPER, TopLeftPopLastHit ? true : false, 0, TopLeftPopLastHit ? 100 : 0);
      RPU_SetLampState(LAMP_TOP_CENTER_POP_BUMPER, TopCenterPopLastHit ? true : false, 0, TopCenterPopLastHit ? 100 : 0);
      RPU_SetLampState(LAMP_TOP_RIGHT_POP_BUMPER, TopRightPopLastHit ? true : false, 0, TopRightPopLastHit ? 100 : 0);
    }

    RPU_SetLampState(LAMP_BOTTOM_POP_BUMPERS, LowerPopStatus[CurrentPlayer], 0, (LowerPopStatus[CurrentPlayer] > 1) ? 500 : 0);
  }

  if ((CurrentTime - TopCenterPopLastHit) > 1500) TopCenterPopLastHit = 0;
  if ((CurrentTime - TopLeftPopLastHit) > 1500) TopLeftPopLastHit = 0;
  if ((CurrentTime - TopRightPopLastHit) > 1500) TopRightPopLastHit = 0;

}

byte SaucerLampNum[4] = {LAMP_SAUCER_2K, LAMP_SAUCER_5K, LAMP_SAUCER_10K, LAMP_SAUCER_EXTRA_BALL};

void ShowSaucerLamps() {
  if (  (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE_START || (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE_ADD_ENEMY || 
        (GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD || (GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD_START ) {
    byte lampPhase = ((CurrentTime - GameModeStartTime) / 100) % 4;
    RPU_SetLampState(LAMP_SAUCER_2K, lampPhase == 0);
    RPU_SetLampState(LAMP_SAUCER_5K, lampPhase == 1);
    RPU_SetLampState(LAMP_SAUCER_10K, lampPhase == 2);
    RPU_SetLampState(LAMP_SAUCER_EXTRA_BALL, lampPhase == 3);
  } else if (IdleMode == IDLE_MODE_ADVERTISE_BATTLE) {
    byte lampPhase = ((CurrentTime - GameModeStartTime) / 250) % 4;
    RPU_SetLampState(LAMP_SAUCER_2K, lampPhase == 0);
    RPU_SetLampState(LAMP_SAUCER_5K, lampPhase == 1);
    RPU_SetLampState(LAMP_SAUCER_10K, lampPhase == 2);
    RPU_SetLampState(LAMP_SAUCER_EXTRA_BALL, lampPhase == 3);
  } else {
    if (SaucerScoreAnimationStart != 0) {
      byte lampPhase = ((CurrentTime - SaucerScoreAnimationStart) / 100) % (SaucerValue[CurrentPlayer] + 1);
      for (byte count = 0; count < 4; count++) {
        RPU_SetLampState(SaucerLampNum[count], (count + 1) == lampPhase);
      }

      if (CurrentTime > (SaucerScoreAnimationStart + 5000)) SaucerScoreAnimationStart = 0;
    } else {
      RPU_SetLampState(LAMP_SAUCER_2K, (SaucerValue[CurrentPlayer] >= SAUCER_VALUE_2K) ? true : false);
      RPU_SetLampState(LAMP_SAUCER_5K, (SaucerValue[CurrentPlayer] >= SAUCER_VALUE_5K) ? true : false);
      RPU_SetLampState(LAMP_SAUCER_10K, (SaucerValue[CurrentPlayer] >= SAUCER_VALUE_10K) ? true : false);
      RPU_SetLampState(LAMP_SAUCER_EXTRA_BALL, (SaucerValue[CurrentPlayer] >= SAUCER_VALUE_EB) ? true : false);
    }
  }

}


////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long LastTimeScoreChanged = 0;
unsigned long LastTimeOverrideAnimated = 0;
unsigned long LastFlashOrDash = 0;
#ifdef USE_SCORE_OVERRIDES
unsigned long ScoreOverrideValue[4] = {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#endif
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}

#ifdef USE_SCORE_OVERRIDES
void OverrideScoreDisplay(byte displayNum, unsigned long value, boolean animate) {
  if (displayNum > 3) return;
  ScoreOverrideStatus |= (0x10 << displayNum);
  if (animate) ScoreOverrideStatus |= (0x01 << displayNum);
  else ScoreOverrideStatus &= ~(0x01 << displayNum);
  ScoreOverrideValue[displayNum] = value;
}
#endif

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount = 0; digitCount < numDigits; digitCount++) {
    displayMask |= (0x20 >> digitCount);
  }
  return displayMask;
}


void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue = 0) {

#ifdef USE_SCORE_OVERRIDES
  if (displayToUpdate == 0xFF) ScoreOverrideStatus = 0;
#endif

  byte displayMask = 0x3F;
  unsigned long displayScore = 0;
  unsigned long overrideAnimationSeed = CurrentTime / 250;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime - LastTimeScoreChanged) / 250) % 16;
  if (scrollPhase != LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  boolean updateLastTimeAnimated = false;

  for (byte scoreCount = 0; scoreCount < 4; scoreCount++) {

#ifdef USE_SCORE_OVERRIDES
    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue == 0 && (ScoreOverrideStatus & (0x10 << scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      if (displayScore != DISPLAY_OVERRIDE_BLANK_SCORE) {
        byte numDigits = MagnitudeOfScore(displayScore);
        if (numDigits == 0) numDigits = 1;
        if (numDigits < (RPU_OS_NUM_DIGITS - 1) && (ScoreOverrideStatus & (0x01 << scoreCount))) {
          // This score is going to be animated (back and forth)
          if (overrideAnimationSeed != LastTimeOverrideAnimated) {
            updateLastTimeAnimated = true;
            byte shiftDigits = (overrideAnimationSeed) % (((RPU_OS_NUM_DIGITS + 1) - numDigits) + ((RPU_OS_NUM_DIGITS - 1) - numDigits));
            if (shiftDigits >= ((RPU_OS_NUM_DIGITS + 1) - numDigits)) shiftDigits = (RPU_OS_NUM_DIGITS - numDigits) * 2 - shiftDigits;
            byte digitCount;
            displayMask = GetDisplayMask(numDigits);
            for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
              displayScore *= 10;
              displayMask = displayMask >> 1;
            }
            RPU_SetDisplayBlank(scoreCount, 0x00);
            RPU_SetDisplay(scoreCount, displayScore, false);
            RPU_SetDisplayBlank(scoreCount, displayMask);
          }
        } else {
          RPU_SetDisplay(scoreCount, displayScore, true, 1);
        }
      } else {
        RPU_SetDisplayBlank(scoreCount, 0);
      }

    } else {
#endif
      // No override, update scores designated by displayToUpdate
      //CurrentScores[CurrentPlayer] = CurrentScoreOfCurrentPlayer;
      if (allScoresShowValue == 0) displayScore = CurrentScores[scoreCount];
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate == 0xFF || displayToUpdate == scoreCount || displayScore > RPU_OS_MAX_DISPLAY_SCORE) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate == 0xFF && (scoreCount >= CurrentNumPlayers && CurrentNumPlayers != 0) && allScoresShowValue == 0) {
          RPU_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore > RPU_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled
          if ((CurrentTime - LastTimeScoreChanged) < 4000) {
            RPU_SetDisplay(scoreCount, displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1), false);
            RPU_SetDisplayBlank(scoreCount, RPU_OS_ALL_DIGITS_MASK);
          } else {

            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase < 11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);

              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase < RPU_OS_NUM_DIGITS) {
                displayMask = RPU_OS_ALL_DIGITS_MASK;
                for (byte scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0;
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits + scrollPhase) > 10) {
                byte numDigitsNeeded = (numDigits + scrollPhase) - 10;
                for (byte scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              RPU_SetDisplayBlank(scoreCount, displayMask);
              RPU_SetDisplay(scoreCount, displayScore);
            }
          }
        } else {
          if (flashCurrent) {
            unsigned long flashSeed = CurrentTime / 250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime / 250) % 2) == 0) RPU_SetDisplayBlank(scoreCount, 0x00);
              else RPU_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent) {
            unsigned long dashSeed = CurrentTime / 50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime / 60) % 36;
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase < 12) {
                displayMask = GetDisplayMask((numDigits == 0) ? 2 : numDigits);
                if (dashPhase < 7) {
                  for (byte maskCount = 0; maskCount < dashPhase; maskCount++) {
                    displayMask &= ~(0x01 << maskCount);
                  }
                } else {
                  for (byte maskCount = 12; maskCount > dashPhase; maskCount--) {
                    displayMask &= ~(0x20 >> (maskCount - dashPhase - 1));
                  }
                }
                RPU_SetDisplay(scoreCount, displayScore);
                RPU_SetDisplayBlank(scoreCount, displayMask);
              } else {
                RPU_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
          } else {
            RPU_SetDisplay(scoreCount, displayScore, true, 2);
          }
        }
      } // End if this display should be updated
#ifdef USE_SCORE_OVERRIDES
    } // End on non-overridden
#endif
  } // End loop on scores

  if (updateLastTimeAnimated) {
    LastTimeOverrideAnimated = overrideAnimationSeed;
  }

}

void ShowFlybyValue(byte numToShow, unsigned long timeBase) {
  byte shiftDigits = (CurrentTime - timeBase) / 120;
  byte rightSideBlank = 0;

  unsigned long bigVersionOfNum = (unsigned long)numToShow;
  for (byte count = 0; count < shiftDigits; count++) {
    bigVersionOfNum *= 10;
    rightSideBlank /= 2;
    if (count > 2) rightSideBlank |= 0x20;
  }
  bigVersionOfNum /= 1000;

  byte curMask = RPU_SetDisplay(CurrentPlayer, bigVersionOfNum, false, 0);
  if (bigVersionOfNum == 0) curMask = 0;
  RPU_SetDisplayBlank(CurrentPlayer, ~(~curMask | rightSideBlank));
}

/*

  XXdddddd---
           10
          100
         1000
        10000
       10x000
      10xx000
     10xxx000
    10xxxx000
   10xxxxx000
  10xxxxxx000
*/

////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  RPU_SetDisplay(CurrentNumPlayers - 1, 0);
  RPU_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  RPU_SetLampState(LAMP_HEAD_1_PLAYER, CurrentNumPlayers==1, 0, 500);
  RPU_SetLampState(LAMP_HEAD_2_PLAYERS, CurrentNumPlayers==2, 0, 500);
  RPU_SetLampState(LAMP_HEAD_3_PLAYERS, CurrentNumPlayers==3, 0, 500);
  RPU_SetLampState(LAMP_HEAD_4_PLAYERS, CurrentNumPlayers==4, 0, 500);

  if (!FreePlayMode) {
    Credits -= 1;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  }
  QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1 + (CurrentNumPlayers - 1), 10);

  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}


unsigned short ChuteAuditByte[] = {RPU_CHUTE_1_COINS_START_BYTE, RPU_CHUTE_2_COINS_START_BYTE, RPU_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum>2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) {
      //PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
      RPU_PushToSolenoidStack(SOL_KNOCKER, 8, true); // Units of this era use the knocker for credits
    }
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  } else {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true);
  }

}

byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit==SW_COIN_2) chuteNum = 1;
  else if (switchHit==SW_COIN_3) chuteNum = 2;
  return chuteNum;   
}

boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum>2) return false;
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse=0; chuteNumToUse<=chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse)==cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_COIN_DROP_1+(CurrentTime%3));

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse]==cpcCoins) {
    if (cpcCredits>cpcCoins) AddCredit(cpcCredits - (coinProgressBefore));
    else AddCredit(cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits>cpcCoins) {
      AddCredit(1);
      creditAdded = true;
    } else {
    }
  }

  return creditAdded;
}


void AddSpecialCredit() {
  AddCredit(false, 1);
  RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 8, CurrentTime, true);
  RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += SpecialValue * PlayfieldMultiplier;
  } else {
    AddSpecialCredit();
  }
}

boolean AwardExtraBall() {
  if (ExtraBallCollected) return false;
  ExtraBallCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ExtraBallValue * PlayfieldMultiplier;
  } else {
    SamePlayerShootsAgain = true;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, SamePlayerShootsAgain);
    QueueNotification(SOUND_EFFECT_VP_EXTRA_BALL, 8);
  }
  return true;
}


void IncreasePlayfieldMultiplier(unsigned long duration) {
  if (PlayfieldMultiplierExpiration) PlayfieldMultiplierExpiration += duration;
  else PlayfieldMultiplierExpiration = CurrentTime + duration;
  PlayfieldMultiplier += 1;
  if (PlayfieldMultiplier > 5) {
    PlayfieldMultiplier = 5;
  } else {
    QueueNotification(SOUND_EFFECT_VP_1X_PLAYFIELD + (PlayfieldMultiplier - 1), 1);
  }
}


#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
unsigned long SoundSettingTimeout = 0;
unsigned long AdjustmentScore;


int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    StopAudio();
    RPU_TurnOffAllLamps();
    PlaySoundEffect(SOUND_EFFECT_SELF_TEST_MODE_START-curState, 0, true);
    if (DEBUG_MESSAGES) {
      char buf[256];
      sprintf(buf, "State changed to %d\n", curState);
      Serial.write(buf);
    }
  } else {
    if (SoundSettingTimeout && CurrentTime>SoundSettingTimeout) {
      SoundSettingTimeout = 0;
      StopAudio();
    }
  }

  // Any state that's greater than MACHINE_STATE_TEST_DONE is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_DONE) {
    byte cpcSelection = 0xFF;
    byte chuteNum = 0xFF;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1) chuteNum = 0;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2) chuteNum = 1;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) chuteNum = 2;
    if (chuteNum!=0xFF) cpcSelection = GetCPCSelection(chuteNum);
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
    if (chuteNum!=0xFF) {
      if (cpcSelection != GetCPCSelection(chuteNum)) {
        byte newCPC = GetCPCSelection(chuteNum);
        PlaySoundEffect(SOUND_EFFECT_SELF_TEST_CPC_START+newCPC, 0, true);
      }
    }
  } else {
    byte curSwitch = RPU_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      if (RPU_GetUpDownSwitchState()) returnState -= 1;
      else returnState += 1;
      if (DEBUG_MESSAGES) {
        char buf[80];
        sprintf(buf, "Up/Down = %d, new mode = %d\n", RPU_GetUpDownSwitchState()?1:0, returnState);
        Serial.write(buf);
      }
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {

      if (DEBUG_MESSAGES) {
        char buf[256];
        sprintf(buf, "We're in game adjustment state %d\n", curState);
        Serial.write(buf);
      }
      
      for (int count = 0; count < 4; count++) {
        RPU_SetDisplay(count, 0);
        RPU_SetDisplayBlank(count, 0x00);
      }
      RPU_SetDisplayCredits(MACHINE_STATE_TEST_BOOT - curState);
      RPU_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SOUND_SELECTOR:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[1] = 9;
          CurrentAdjustmentByte = &SoundSelector;
          CurrentAdjustmentStorageByte = EEPROM_SOUND_SELECTOR_BYTE;
          break;
        case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &MusicVolume;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SFX_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &SoundEffectsVolume;
          CurrentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &CalloutsVolume;
          CurrentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 3;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 99;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;
        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_UL;
          break;
        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_UL;
          break;
        case MACHINE_STATE_ADJUST_GOALS_UNTIL_WIZARD:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &GoalsUntilWizard;
          CurrentAdjustmentStorageByte = EEPROM_GOALS_UNTIL_WIZ_BYTE;
          break;
        case MACHINE_STATE_ADJUST_WIZARD_TIME:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 4;
          AdjustmentValues[0] = 20;
          AdjustmentValues[1] = 30;
          AdjustmentValues[2] = 45;
          AdjustmentValues[3] = 60;
          CurrentAdjustmentByte = &WizardModeTime;
          CurrentAdjustmentStorageByte = EEPROM_WIZ_TIME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_IDLE_MODE:
          CurrentAdjustmentByte = (byte *)&IdleModeOn;
          CurrentAdjustmentStorageByte = EEPROM_IDLE_MODE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_COMBOS_TO_FINISH:
          AdjustmentValues[0] = 4;
          AdjustmentValues[1] = 6;
          CurrentAdjustmentByte = &CombosToFinishGoal;
          CurrentAdjustmentStorageByte = EEPROM_COMBOS_GOAL_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SPINNER_ACCELERATORS:
          CurrentAdjustmentByte = (byte *)&SpinnerAccelerators;
          CurrentAdjustmentStorageByte = EEPROM_SPINNER_ACCELERATOR_BYTE;
          break;
        case MACHINE_STATE_ADJUST_ALLOW_RESET:
          CurrentAdjustmentByte = (byte *)&AllowResetAfterBallOne;
          CurrentAdjustmentStorageByte = EEPROM_ALLOW_RESET_BYTE;
          break;
          
        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }
    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;

        if (RPU_GetUpDownSwitchState()) {
          curVal += 1;
          if (curVal > AdjustmentValues[1]) {
            if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
            else {
              if (curVal > 99) curVal = AdjustmentValues[0];
              else curVal = 99;
            }
          }
        } else {
          if (curVal==AdjustmentValues[0]) {            
            if (AdjustmentType==ADJ_TYPE_MIN_MAX_DEFAULT) curVal = 99;
            else curVal = AdjustmentValues[1];
          } else {
            curVal -= 1;
          }
        }

        if (DEBUG_MESSAGES) {
          char buf[80];
          sprintf(buf, "Up/Down = %d\n", RPU_GetUpDownSwitchState()?1:0);
          Serial.write(buf);
        }

        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);

        if (curState==MACHINE_STATE_ADJUST_SOUND_SELECTOR) {
          StopAudio();
          PlaySoundEffect(SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START+curVal, 0, true);
        } else if (curState==MACHINE_STATE_ADJUST_MUSIC_VOLUME) {
          if (SoundSettingTimeout) StopAudio();
          PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1);
          SoundSettingTimeout = CurrentTime+5000;
        } else if (curState==MACHINE_STATE_ADJUST_SFX_VOLUME) {
          if (SoundSettingTimeout) StopAudio();
          PlaySoundEffect(SOUND_EFFECT_SW_LETTER_AWARDED, ConvertVolumeSettingToGain(SoundEffectsVolume), true);
          SoundSettingTimeout = CurrentTime+5000;
        } else if (curState==MACHINE_STATE_ADJUST_CALLOUTS_VOLUME) {
          if (SoundSettingTimeout) StopAudio();
          PlaySoundEffect(SOUND_EFFECT_VP_INTRUDER_ALERT, ConvertVolumeSettingToGain(CalloutsVolume), true);
          SoundSettingTimeout = CurrentTime+3000;
        }
        
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        boolean upDownState = RPU_GetUpDownSwitchState();
        for (valCount = 0; valCount < (NumAdjustmentValues); valCount++) {
          if (curVal == AdjustmentValues[valCount]) {
            if (upDownState) {
              if (valCount<(NumAdjustmentValues-1)) newIndex = valCount + 1;
            } else {
              if (valCount>0) newIndex = valCount - 1;
            }
          }
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        if (RPU_GetUpDownSwitchState()) curVal += 5000;
        else if (curVal>=5000) curVal -= 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) RPU_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      RPU_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      RPU_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
    //    DecodeDIPSwitchParameters();
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    ReadStoredParameters();
  }

  return returnState;
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////
int VolumeToGainConversion[] = { -70, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0};
int ConvertVolumeSettingToGain(byte volumeSetting) {
  if (volumeSetting == 0) return -70;
  if (volumeSetting > 10) return 0;
  return VolumeToGainConversion[volumeSetting];
}

#define VOICE_NOTIFICATION_STACK_SIZE   10
#define VOICE_NOTIFICATION_STACK_EMPTY  0xFFFF
byte VoiceNotificationStackFirst = 0;
byte VoiceNotificationStackLast = 0;
unsigned int VoiceNotificationNumStack[VOICE_NOTIFICATION_STACK_SIZE];
byte VoiceNotificationPriorityStack[VOICE_NOTIFICATION_STACK_SIZE];
unsigned int CurrentNotificationPlaying = 0;
byte CurrentNotificationPriority = 0;

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
unsigned short CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif

void StopAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  wTrig.stopAllTracks();
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif
  VoiceNotificationStackFirst = 0;
  VoiceNotificationStackLast = 0;

}

void StopBackgroundSong() {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
    wTrig.trackFade(CurrentBackgroundSong, -70, 2000, 1);
    CurrentBackgroundSong = SOUND_EFFECT_NONE;
  }
#endif
}

void PlayBackgroundSong(unsigned int songNum, unsigned long backgroundSongNumSeconds) {

  if (MusicVolume==0) return;

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  if (SoundSelector==3 || SoundSelector==4 || SoundSelector==6 || SoundSelector==7) {
    if (CurrentBackgroundSong != songNum) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);

      if (0 && DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Stopping song %d to play song %d\n", CurrentBackgroundSong, songNum);
        Serial.write(buf);
      }

      if (songNum != SOUND_EFFECT_NONE) {
#ifdef RPU_OS_USE_WAV_TRIGGER_1p3
        wTrig.trackPlayPoly(songNum, true);
#else
        wTrig.trackPlayPoly(songNum);
#endif
        wTrig.trackLoop(songNum, true);
        wTrig.trackGain(songNum, ConvertVolumeSettingToGain(MusicVolume));
      }
      CurrentBackgroundSong = songNum;

      if (backgroundSongNumSeconds != 0) {
        BackgroundSongEndTime = CurrentTime + (unsigned long)(backgroundSongNumSeconds) * 1000;
      } else {
        BackgroundSongEndTime = 0;
      }

    }
  }
#else
  // This is just nonsense to eliminate the compiler warnings
  // when compiling without WAV Trigger support
  byte test = songNum;
  songNum = test;
  unsigned long test2 = backgroundSongNumSeconds;
  backgroundSongNumSeconds = test2;
#endif

  if (0 && DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Song # %d\n", songNum);
    Serial.write(buf);
  }

}

// These SoundEFfectEntry & Queue functions parcel out FX to the
// built-in sound card because it can only handle one sound
// at a time.
struct SoundEffectEntry {
  unsigned short soundEffectNum;
  unsigned long requestedPlayTime;
  unsigned long playUntil;
  byte priority; // 0 is least important, 100 is most
  boolean inUse;
};

#define SOUND_EFFECT_QUEUE_SIZE 50
SoundEffectEntry CurrentSoundPlaying;
SoundEffectEntry SoundEffectQueue[SOUND_EFFECT_QUEUE_SIZE];

void InitSoundEffectQueue() {
  CurrentSoundPlaying.soundEffectNum = 0;
  CurrentSoundPlaying.requestedPlayTime = 0;
  CurrentSoundPlaying.playUntil = 0;
  CurrentSoundPlaying.priority = 0;
  CurrentSoundPlaying.inUse = false;

  for (byte count = 0; count < SOUND_EFFECT_QUEUE_SIZE; count++) {
    SoundEffectQueue[count].soundEffectNum = 0;
    SoundEffectQueue[count].requestedPlayTime = 0;
    SoundEffectQueue[count].playUntil = 0;
    SoundEffectQueue[count].priority = 0;
    SoundEffectQueue[count].inUse = false;
  }
}

boolean PlaySoundEffectWhenPossible(unsigned short soundEffectNum, unsigned long requestedPlayTime = 0, unsigned long playUntil = 50, byte priority = 10) {
  byte count = 0;
  for (count = 0; count < SOUND_EFFECT_QUEUE_SIZE; count++) {
    if (SoundEffectQueue[count].inUse == false) break;
  }
  if (count == SOUND_EFFECT_QUEUE_SIZE) return false;
  SoundEffectQueue[count].soundEffectNum = soundEffectNum;
  SoundEffectQueue[count].requestedPlayTime = requestedPlayTime + CurrentTime;
  SoundEffectQueue[count].playUntil = playUntil + requestedPlayTime + CurrentTime;
  SoundEffectQueue[count].priority = priority;
  SoundEffectQueue[count].inUse = true;

  if (0 && DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Sound 0x%04X slotted at %d\n\r", soundEffectNum, count);
    Serial.write(buf);
  }
  return true;
}

unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(unsigned int soundEffectNum, int gain, boolean overrideSelector) {

  if (SoundSelector == 0 && !overrideSelector) return;
  
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  // Only use wav trigger sounds if we're above option 5
  if (overrideSelector || (SoundSelector>5 && soundEffectNum<SOUND_EFFECT_WAV_MANDATORY) || soundEffectNum>=SOUND_EFFECT_WAV_MANDATORY) {
#ifndef RPU_OS_USE_WAV_TRIGGER_1p3
    if (  soundEffectNum==SOUND_EFFECT_LEFT_SPINNER ||
          soundEFfectNum==SOUND_EFFECT_RIGHT_SPINNER ||
          soundEFfectNum==SOUND_EFFECT_BUMPER_HIT ||
          soundEFfectNum==SOUND_EFFECT_FRENZY_BUMPER_HIT ) {
      wTrig.trackStop(soundEffectNum);
    }
#endif
    wTrig.trackPlayPoly(soundEffectNum);
    if (overrideSelector) {
      // Don't mess with the gain
    } else if (gain!=1000) {
      wTrig.trackGain(soundEffectNum, gain);
    } else {
      wTrig.trackGain(soundEffectNum, ConvertVolumeSettingToGain(SoundEffectsVolume));
    }
  }
#else
  int test = gain; // This nonsense is to prevent compiler warnings
  gain = test;
#endif

#if defined(RPU_TYPE_1_SOUND)
  // Only use Type 1 sound if we're below option 6
  if (SoundSelector<6) {
    switch (soundEffectNum) {
//      case SOUND_EFFECT_ADD_CREDIT:
//        RPU_PushToSolenoidStack(SOL_KNOCKER, 8, true);
//        break;
      case SOUND_EFFECT_SLING_SHOT:
        PlaySoundEffectWhenPossible(7 * 256, 0, 55, 5);
        PlaySoundEffectWhenPossible(19 * 256, 60, 15, 5);
        break;
      case SOUND_EFFECT_SCORE_TICK:
        PlaySoundEffectWhenPossible(7 * 256, 0, 75, 5);
        PlaySoundEffectWhenPossible(19 * 256, 80, 15, 5);
        break;
      case SOUND_EFFECT_TILT_WARNING:
        PlaySoundEffectWhenPossible(6 * 256, 0, 1900, 100);
        PlaySoundEffectWhenPossible(19 * 256, 2000, 15, 100);
        break;
      case SOUND_EFFECT_TILT:
        PlaySoundEffectWhenPossible(6 * 256, 0, 1900, 100);
        PlaySoundEffectWhenPossible(8 * 256, 2000, 500, 100);
        break;
      case SOUND_EFFECT_OUTLANE_UNLIT:
        PlaySoundEffectWhenPossible(30 * 256, 0, 150, 90);
        PlaySoundEffectWhenPossible(29 * 256, 175, 150, 90);
        PlaySoundEffectWhenPossible(28 * 256, 350, 150, 90);
        PlaySoundEffectWhenPossible(27 * 256, 525, 150, 90);
        PlaySoundEffectWhenPossible(26 * 256, 700, 150, 90);
        PlaySoundEffectWhenPossible(25 * 256, 875, 150, 90);
        PlaySoundEffectWhenPossible(7 * 256, 1050, 150, 90);
        break;
      case SOUND_EFFECT_BATTLE_START:
        PlaySoundEffectWhenPossible(16 * 256, 0, 500, 90);
        PlaySoundEffectWhenPossible(12 * 256, 1500, 500, 90);
        PlaySoundEffectWhenPossible(12 * 256, 2500, 500, 90);
        PlaySoundEffectWhenPossible(12 * 256, 3500, 500, 90);
        break;
      case SOUND_EFFECT_BUMPER_HIT:
        PlaySoundEffectWhenPossible(5 * 256);
        break;
      case SOUND_EFFECT_FRENZY_BUMPER_HIT:
        PlaySoundEffectWhenPossible(10 * 256);
        break;
      case SOUND_EFFECT_NEUTRAL_ZONE_DUPLICATE:
        PlaySoundEffectWhenPossible(13 * 256);
        break;
      case SOUND_EFFECT_NEUTRAL_ZONE_HIT:
        PlaySoundEffectWhenPossible(15 * 256);
        break;
      case SOUND_EFFECT_LOWER_BUMPER_HIT:
        PlaySoundEffectWhenPossible(10 * 256);
        break;
//      case SOUND_EFFECT_10PT_SWITCH:
//        PlaySoundEffectWhenPossible(3 * 256);
//        break;
      case SOUND_EFFECT_BULLSEYE_LIT:
        PlaySoundEffectWhenPossible(10 * 256);
        break;
      case SOUND_EFFECT_BULLSEYE_UNLIT:
      case SOUND_EFFECT_CAPTIVE_BALL_UNLIT:
        PlaySoundEffectWhenPossible(3 * 256);
        break;
      case SOUND_EFFECT_MATCH_SPIN:
      case SOUND_EFFECT_LEFT_SPINNER:
        PlaySoundEffectWhenPossible(10 * 256, 0, 50, 8);
        break;
      case SOUND_EFFECT_RIGHT_SPINNER:
        PlaySoundEffectWhenPossible(20 * 256, 0, 50, 8);
        break;
      case SOUND_EFFECT_BONUS_COUNT:
        PlaySoundEffectWhenPossible(20 * 256, 0, 50, 90);
        break;
      case SOUND_EFFECT_SR_FINISHED:
      case SOUND_EFFECT_WS_FINISHED:
        PlaySoundEffectWhenPossible(20 * 256, 0, 50, 90);
        break;
      case SOUND_EFFECT_SRWS_FINISHED:
        PlaySoundEffectWhenPossible(7 * 256, 0, 50, 90);
        PlaySoundEffectWhenPossible(7 * 256, 400, 50, 91);
        PlaySoundEffectWhenPossible(7 * 256, 600, 50, 92);
        PlaySoundEffectWhenPossible(8 * 256, 900, 300, 93);
        break;
      case SOUND_EFFECT_MACHINE_START:
        if (SoundSelector==1) {
          PlaySoundEffectWhenPossible(17 * 256, 0, 20, 100);
          PlaySoundEffectWhenPossible(25 * 256, 50, 150, 100);
          PlaySoundEffectWhenPossible(30 * 256, 250, 150, 100);
          PlaySoundEffectWhenPossible(17 * 256, 500, 150, 100);
          PlaySoundEffectWhenPossible(27 * 256, 750, 150, 100);
          PlaySoundEffectWhenPossible(30 * 256, 1000, 150, 100);
          PlaySoundEffectWhenPossible(29 * 256, 1125, 150, 100);
          PlaySoundEffectWhenPossible(18 * 256, 1250, 150, 100);
          PlaySoundEffectWhenPossible(17 * 256, 1750, 150, 100);
          PlaySoundEffectWhenPossible(18 * 256, 2125, 150, 100);
          PlaySoundEffectWhenPossible(19 * 256, 2350, 20, 100);
          PlaySoundEffectWhenPossible(22 * 256, 2375, 150, 100);
        }
        break;
      case SOUND_EFFECT_TOP_LANE_REPEAT:
        PlaySoundEffectWhenPossible(13 * 256, 0, 50, 8);
        break;
      case SOUND_EFFECT_TOP_LANE_NEW:
        PlaySoundEffectWhenPossible(15 * 256, 0, 50, 8);
        break;
      case SOUND_EFFECT_SKILL_SHOT:
        PlaySoundEffectWhenPossible(19 * 256, 0, 10, 100);
        PlaySoundEffectWhenPossible(22 * 256, 250, 50, 8);
        break;
      case SOUND_EFFECT_TOP_LANE_LEVEL_FINISHED:
        PlaySoundEffectWhenPossible(22 * 256, 250, 50, 8);
        break;
      case SOUND_EFFECT_SW_LETTER_AWARDED:
        PlaySoundEffectWhenPossible(2 * 256, 0, 50, 75);
        break;
      case SOUND_EFFECT_DROP_TARGET_HIT:
        PlaySoundEffectWhenPossible(4 * 256, 0, 50, 50);
        break;
      case SOUND_EFFECT_DROP_TARGET_RESET:
        PlaySoundEffectWhenPossible(7 * 256, 750, 50, 50);
        break;
      case SOUND_EFFECT_WIZARD_START:
      case SOUND_EFFECT_BATTLE_ADD_ENEMY:
        PlaySoundEffectWhenPossible(11 * 256, 0, 2000, 90);
        PlaySoundEffectWhenPossible(19 * 256, 2100, 100, 95);
        break;
      case SOUND_EFFECT_BATTLE_LOST:
        PlaySoundEffectWhenPossible(8 * 256, 0, 2000, 90);
        break;
      case SOUND_EFFECT_BATTLE_WON:
        // Need an integrated light and sound show
        PlaySoundEffectWhenPossible(24 * 256, 1000, 3000, 90);
        PlaySoundEffectWhenPossible(7 * 256, 3100, 3300, 90);
        break;
//      case SOUND_EFFECT_WAITING_FOR_SKILL:
        /*
              PlaySoundEffectWhenPossible(17*256, 0, 500, 10);
              PlaySoundEffectWhenPossible(18*256, 500, 190, 10);
              PlaySoundEffectWhenPossible(19*256, 700, 10, 100);
        */
        /*
              PlaySoundEffectWhenPossible(28*256, 0, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 800, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 1600, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 2400, 100, 10);
              PlaySoundEffectWhenPossible(29*256, 2800, 100, 10);
              PlaySoundEffectWhenPossible(30*256, 3000, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 3200, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 4000, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 4800, 100, 10);
              PlaySoundEffectWhenPossible(28*256, 5600, 100, 10);
              PlaySoundEffectWhenPossible(29*256, 6000, 100, 10);
              PlaySoundEffectWhenPossible(30*256, 6200, 100, 10);
        */
//        break;
      case SOUND_EFFECT_INVADING_ENEMY_HIT:
        PlaySoundEffectWhenPossible(7 * 256, 0, 190, 90);
        break;
      case SOUND_EFFECT_INVADING_ENEMY_MISS:
        PlaySoundEffectWhenPossible(13 * 256, 0, 50, 90);
        break;
      case SOUND_EFFECT_BATTLE_ENEMY_HIT:
        PlaySoundEffectWhenPossible(7 * 256, 0, 190, 90);
        PlaySoundEffectWhenPossible(7 * 256, 200, 190, 90);
        PlaySoundEffectWhenPossible(7 * 256, 400, 190, 90);
        PlaySoundEffectWhenPossible(22 * 256, 600, 2000, 90);
        break;
      case SOUND_EFFECT_SHIELD_DESTROYED:
        PlaySoundEffectWhenPossible(22 * 256, 500, 450, 90);
        PlaySoundEffectWhenPossible(22 * 256, 1000, 450, 90);
        PlaySoundEffectWhenPossible(22 * 256, 1500, 450, 90);
        PlaySoundEffectWhenPossible(22 * 256, 2000, 450, 90);
        PlaySoundEffectWhenPossible(22 * 256, 2500, 450, 90);
        PlaySoundEffectWhenPossible(22 * 256, 3000, 450, 90);
        break;
      case SOUND_EFFECT_ENEMY_INVASION_ALARM:
        PlaySoundEffectWhenPossible(21 * 256, 0, 900, 90);
        PlaySoundEffectWhenPossible(19 * 256, 1000, 50, 10);
        break;
      case SOUND_EFFECT_WIZARD_START_SAUCER:
        PlaySoundEffectWhenPossible(22 * 256, 0, 50, 90);
        break;
    }
  }
#endif

}

inline void StopSoundEffect(byte soundEffectNum) {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  wTrig.trackStop(soundEffectNum);
#else
  if (0 && DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }
#endif
}


void UpdateSoundQueue() {
  byte highestPrioritySound = 0xFF;
  byte queuePriority = 0;

  for (byte count = 0; count < SOUND_EFFECT_QUEUE_SIZE; count++) {
    // Skip sounds that aren't in use
    if (SoundEffectQueue[count].inUse == false) continue;

    // If a sound has expired, flush it
    if (CurrentTime > SoundEffectQueue[count].playUntil) {
      if (0 && DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Expiring sound in slot %d (CurrentTime=%lu > PlayUntil=%lu)\n\r", count, CurrentTime, SoundEffectQueue[count].playUntil);
        Serial.write(buf);
      }
      SoundEffectQueue[count].inUse = false;
    } else if (CurrentTime > SoundEffectQueue[count].requestedPlayTime) {
      // If this sound is ready to be played, figure out its priority
      if (SoundEffectQueue[count].priority > queuePriority) {
        queuePriority = SoundEffectQueue[count].priority;
        highestPrioritySound = count;
      } else if (SoundEffectQueue[count].priority == queuePriority) {
        if (highestPrioritySound != 0xFF) {
          if (SoundEffectQueue[highestPrioritySound].requestedPlayTime > SoundEffectQueue[count].requestedPlayTime) {
            // The priorities are equal, but this sound was requested before, so switch to it
            highestPrioritySound = count;
          }
        }
      }
    }
  }

  if (CurrentSoundPlaying.inUse && (CurrentTime > CurrentSoundPlaying.playUntil)) {
    CurrentSoundPlaying.inUse = false;
  }

  if (highestPrioritySound != 0xFF) {

    /*
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Ready to play sound 0x%04X\n\r", SoundEffectQueue[highestPrioritySound].soundEffectNum);
          Serial.write(buf);
        }
    */
    if (CurrentSoundPlaying.inUse == false || (CurrentSoundPlaying.inUse && CurrentSoundPlaying.priority < queuePriority)) {
      // Play new sound
      CurrentSoundPlaying.soundEffectNum = SoundEffectQueue[highestPrioritySound].soundEffectNum;
      CurrentSoundPlaying.requestedPlayTime = SoundEffectQueue[highestPrioritySound].requestedPlayTime;
      CurrentSoundPlaying.playUntil = SoundEffectQueue[highestPrioritySound].playUntil;
      CurrentSoundPlaying.priority = SoundEffectQueue[highestPrioritySound].priority;
      CurrentSoundPlaying.inUse = true;
      SoundEffectQueue[highestPrioritySound].inUse = false;
      RPU_PushToSoundStack(CurrentSoundPlaying.soundEffectNum, 8);
    }
  }
}




int SpaceLeftOnNotificationStack() {
  if (VoiceNotificationStackFirst >= VOICE_NOTIFICATION_STACK_SIZE || VoiceNotificationStackLast >= VOICE_NOTIFICATION_STACK_SIZE) return 0;
  if (VoiceNotificationStackLast >= VoiceNotificationStackFirst) return ((VOICE_NOTIFICATION_STACK_SIZE - 1) - (VoiceNotificationStackLast - VoiceNotificationStackFirst));
  return (VoiceNotificationStackFirst - VoiceNotificationStackLast) - 1;
}


void PushToNotificationStack(unsigned int notification, byte priority) {
  // If the switch stack last index is out of range, then it's an error - return
  if (SpaceLeftOnNotificationStack() == 0) return;

  VoiceNotificationNumStack[VoiceNotificationStackLast] = notification;
  VoiceNotificationPriorityStack[VoiceNotificationStackLast] = priority;

  VoiceNotificationStackLast += 1;
  if (VoiceNotificationStackLast == VOICE_NOTIFICATION_STACK_SIZE) {
    // If the end index is off the end, then wrap
    VoiceNotificationStackLast = 0;
  }
}


unsigned int PullFirstFromVoiceNotificationStack(byte *priority) {
  // If first and last are equal, there's nothing on the stack
  if (VoiceNotificationStackFirst == VoiceNotificationStackLast) return VOICE_NOTIFICATION_STACK_EMPTY;

  unsigned int retVal = VoiceNotificationNumStack[VoiceNotificationStackFirst];
  *priority = VoiceNotificationPriorityStack[VoiceNotificationStackFirst];

  VoiceNotificationStackFirst += 1;
  if (VoiceNotificationStackFirst >= VOICE_NOTIFICATION_STACK_SIZE) VoiceNotificationStackFirst = 0;

  return retVal;
}


byte GetTopNotificationPriority() {
  byte startStack = VoiceNotificationStackFirst;
  byte endStack = VoiceNotificationStackLast;
  if (startStack==endStack) return 0;

  byte topPriorityFound = 0;

  while (startStack!=endStack) {
    if (VoiceNotificationPriorityStack[startStack]>topPriorityFound) topPriorityFound = VoiceNotificationPriorityStack[startStack];
    startStack += 1;
    if (startStack >= VOICE_NOTIFICATION_STACK_SIZE) startStack = 0;
  }

  return topPriorityFound;
}


void ClearNotificationStack() {
  VoiceNotificationStackFirst = 0;
  VoiceNotificationStackLast = 0;
}


void StopCurrentNotification() {
  NextVoiceNotificationPlayTime = 0;

  if (CurrentNotificationPlaying!=0) {
    wTrig.trackStop(CurrentNotificationPlaying);
    CurrentNotificationPlaying = 0;
    CurrentNotificationPriority = 0;
  }
}


void QueueNotification(unsigned int soundEffectNum, byte priority) {
  if (CalloutsVolume==0) return;
#if defined (RPU_OS_USE_WAV_TRIGGER) || defined (RPU_OS_USE_WAV_TRIGGER_1p3)
  if (SoundSelector<3 || SoundSelector==4 || SoundSelector==7 || SoundSelector==9) return; 
  if (soundEffectNum < SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START || soundEffectNum >= (SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START + NUM_VOICE_NOTIFICATIONS)) return;

  // if everything on the queue has a lower priority, kill all those
  byte topQueuePriority = GetTopNotificationPriority();
  if (priority>topQueuePriority) {
    ClearNotificationStack();  
  }
  if (priority>CurrentNotificationPriority) {
    StopCurrentNotification();
  }

  // If there's nothing playing, we can play it now
  if (NextVoiceNotificationPlayTime == 0) {
    if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
      wTrig.trackFade(CurrentBackgroundSong, ConvertVolumeSettingToGain(MusicVolume) - 20, 500, 0);
    }
    NextVoiceNotificationPlayTime = CurrentTime + (unsigned long)(VoiceNotificationDurations[soundEffectNum - SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START]) * 1000;
    PlaySoundEffect(soundEffectNum, ConvertVolumeSettingToGain(CalloutsVolume));
    CurrentNotificationPlaying = soundEffectNum;
    CurrentNotificationPriority = priority;
  } else {
    PushToNotificationStack(soundEffectNum, priority);
  }
#else
  unsigned int test = soundEffectNum; // this nonsense is to prevent compiler warnings
  soundEffectNum = test;
  byte test1 = priority; // this nonsense is to prevent compiler warnings
  priority = test1;
#endif
}


void ServiceNotificationQueue() {
#if defined (RPU_OS_USE_WAV_TRIGGER) || defined (RPU_OS_USE_WAV_TRIGGER_1p3)
  if (NextVoiceNotificationPlayTime != 0 && CurrentTime > NextVoiceNotificationPlayTime) {
    // Current notification done, see if there's another
    byte nextPriority = 0;
    unsigned int nextNotification = PullFirstFromVoiceNotificationStack(&nextPriority);
    if (nextNotification != VOICE_NOTIFICATION_STACK_EMPTY) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
        wTrig.trackFade(CurrentBackgroundSong, ConvertVolumeSettingToGain(MusicVolume) - 20, 500, 0);
      }
      NextVoiceNotificationPlayTime = CurrentTime + (unsigned long)(VoiceNotificationDurations[nextNotification - SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START]) * 1000;
      PlaySoundEffect(nextNotification, ConvertVolumeSettingToGain(CalloutsVolume));
      CurrentNotificationPlaying = nextNotification;
      CurrentNotificationPriority = nextPriority;
    } else {
      // No more notifications -- set the volume back up and clear the variable
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
        wTrig.trackFade(CurrentBackgroundSong, ConvertVolumeSettingToGain(MusicVolume), 1500, 0);
      }
      NextVoiceNotificationPlayTime = 0;
      CurrentNotificationPlaying = 0;
      CurrentNotificationPriority = 0;
    }
  }
#endif
}








////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractDisplayRampStart = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;


int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    for (byte count = 0; count < 4; count++) {
      RPU_SetLampState(LAMP_HEAD_PLAYER_1_UP + count, 0);
    }

    RPU_SetLampState(LAMP_HEAD_1_PLAYER, 0);
    RPU_SetLampState(LAMP_HEAD_2_PLAYERS, 0);
    RPU_SetLampState(LAMP_HEAD_3_PLAYERS, 0);
    RPU_SetLampState(LAMP_HEAD_4_PLAYERS, 0);
  }

  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      ShowPlayerScores(0xFF, false, false);
      RPU_SetDisplayCredits(Credits, !FreePlayMode);
      RPU_SetDisplayBallInPlay(0, true);
    }
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 1, 0, 250);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 5);

  if (attractPlayfieldPhase != AttractLastPlayfieldMode) {
    RPU_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase == 2) GameMode = GAME_MODE_SKILL_SHOT;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
    AttractLastLadderBonus = 1;
    AttractLastLadderTime = CurrentTime;
  }

  if (attractPlayfieldPhase < 2) {
    ShowLampAnimation(1, 40, CurrentTime, 14, false, false);
  } else if (attractPlayfieldPhase == 3) {
    ShowLampAnimation(0, 40, CurrentTime, 11, false, false);
  } else if (attractPlayfieldPhase == 2) {
    ShowLampAnimation(1, 40, CurrentTime, 3, false, true);
  } else {
    ShowLampAnimation(2, 40, CurrentTime, 14, false, false);
  }

  byte switchHit;
  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_BOOT;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(unsigned short intToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}


void AddToBonus(byte amountToAdd = 1) {
  CurrentBonus += amountToAdd;
  if (CurrentBonus >= MAX_DISPLAY_BONUS) {
    CurrentBonus = MAX_DISPLAY_BONUS;
  }
}


void SetGameMode(byte newGameMode) {
  GameMode = newGameMode | (GameMode & ~GAME_BASE_MODE);
  GameModeStartTime = 0;
  GameModeEndTime = 0;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Game mode set to %d\n", newGameMode);
    Serial.write(buf);
  }
}

void StartScoreAnimation(unsigned long scoreToAnimate) {
  if (ScoreAdditionAnimation != 0) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
  }
  ScoreAdditionAnimation = scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
}






void IncreaseBonusX() {
  boolean soundPlayed = false;
  if (BonusX[CurrentPlayer] < 14) {
    BonusX[CurrentPlayer] += 1;
    BonusXAnimationStart = CurrentTime;

    if (BonusX[CurrentPlayer] == 13) {
      BonusX[CurrentPlayer] = 14;
      QueueNotification(SOUND_EFFECT_VP_BONUSX_MAX, 2);
    } else {
      QueueNotification(SOUND_EFFECT_VP_BONUSX_INCREASED, 1);
    }
  }


  if (!soundPlayed) {
    //    PlaySoundEffect(SOUND_EFFECT_BONUS_X_INCREASED);
  }

}

boolean WaitForBallToReachOuthole = false;

int InitGamePlay() {

  // Before we start the game, we have to make sure the ball
  // isn't in the saucer
  if (WaitForBallToReachOuthole) {
    if (!RPU_ReadSingleSwitchState(SW_OUTHOLE)) return MACHINE_STATE_INIT_GAMEPLAY;
    WaitForBallToReachOuthole = false;
  } else {
    if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
      RPU_PushToSolenoidStack(SOL_SAUCER, 16, true);
      SaucerEjectTime = CurrentTime;
      WaitForBallToReachOuthole = true;
      if (DEBUG_MESSAGES) {
        Serial.write("Ball is in the saucer - have to wait for it\n\r");
      }
      return MACHINE_STATE_INIT_GAMEPLAY;
    }
  }

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  RPU_EnableSolenoidStack();
  RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);
  RPU_TurnOffAllLamps();
  StopAudio();

  // Reset displays & game state variables
  for (int count = 0; count < 4; count++) {
    // Initialize game-specific variables
    BonusX[count] = 1;
    SaucerValue[count] = 0;
    TotalSpins[count] = 0;
    Bonus[count] = 0;
    TopLaneStatus[count] = 0;
    SWStatus[count] = 0;
    NumLeftDTClears[count] = 0;
    NumCenterDTClears[count] = 0;
    NumRightDTClears[count] = 0;
    OutlaneSpecialLit[count] = false;
    CaptiveBallLit[count] = false;
    BullseyeSpecialLit[count] = false;
    LowerPopStatus[count] = 0;
    CombosAchieved[count] = 0;
    HoldoverAwards[count] = 0x00;
    SWLettersLevel[count] = 0;
    NeutralZoneHits[count] = 0;
    WizardGoals[count] = 0;    
    UsedWizardGoals[count] = 0;
    NumCarryWizardGoals[count] = 0;
  }
  memset(CurrentScores, 0, 4 * sizeof(unsigned long));

  SamePlayerShootsAgain = false;
  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;
  ShowPlayerScores(0xFF, false, false);


  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    RPU_TurnOffAllLamps();
    BallFirstSwitchHitTime = 0;

    // Choose a random lane for the skill shot
    if (playerNum == 0) SkillShotLane = CurrentTime % 4;

    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack();
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    if (CurrentNumPlayers > 1 && (ballNum != 1 || playerNum != 0) && !SamePlayerShootsAgain) QueueNotification(SOUND_EFFECT_VP_PLAYER_1_UP + playerNum, 1);
    SamePlayerShootsAgain = false;

    RPU_SetDisplayBallInPlay(ballNum);
    RPU_SetLampState(LAMP_HEAD_TILT, 0);

    if (BallSaveNumSeconds > 0) {
      RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
      RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, 1, 0, 500);
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT;

    ExtraBallCollected = false;
    SpecialCollected = false;

    // Reset progress unless holdover awards
    if ((HoldoverAwards[CurrentPlayer]&HOLDOVER_BONUS) == 0x00) Bonus[CurrentPlayer] = 0;
    if ((HoldoverAwards[CurrentPlayer]&HOLDOVER_BONUS_X) == 0x00) BonusX[CurrentPlayer] = 1;

    PlayfieldMultiplier = 1;
    PlayfieldMultiplierExpiration = 0;
    LastInlaneHitTime = 0;
    CurrentBonus = Bonus[CurrentPlayer];
    ScoreAdditionAnimation = 0;
    ScoreAdditionAnimationStartTime = 0;
    BonusXAnimationStart = 0;
    LastSpinnerHit = 0;
    BasesVisited = 0;
    TopLaneStatus[CurrentPlayer] &= 0xF0;

    TopCenterPopLastHit = 0;
    TopLeftPopLastHit = 0;
    TopRightPopLastHit = 0;

    SaucerScoreAnimationStart = 0;
    BattleLetter = 0;
    UpperPopFrenzyFinish = 0;
    SpinnerFrenzyEndTime = 0;
    NextVoiceNotificationPlayTime = 0;
    InvasionPosition = INVASION_POSITION_NONE;
    ShieldDestroyedAnimationStart = 0;
    BallSaveEndTime = 0;
    IdleMode = IDLE_MODE_NONE;

    LastLeftInlane = 0;
    LastRightInlane = 0;

    for (byte count = 0; count < NUM_BALL_SEARCH_SOLENOIDS; count++) {
      BallSearchSolenoidFireTime[count] = 0;
    }

    for (byte count = 0; count < 4; count++) {
      TopLaneAnimationStartTime[count] = 0;
    }

    for (byte count = 0; count < 11; count++) {
      SWLettersAnimationStartTime[count] = 0;
      SWLettersExpirationTime[count] = 0;
    }

    if (NumCenterDTClears[CurrentPlayer]>3) {
      NumCenterDTClears[CurrentPlayer] = 2;
      SaucerValue[CurrentPlayer] = SAUCER_VALUE_5K;
    }

    // Reset Drop Targets
    RPU_PushToTimedSolenoidStack(SOL_LEFT_DT_RESET, 50, CurrentTime + 100);
    ResetLeftDropTargetStatusTime = CurrentTime + 200;
    RPU_PushToTimedSolenoidStack(SOL_CENTER_LEFT_DT_RESET, 50, CurrentTime + 225);
    RPU_PushToTimedSolenoidStack(SOL_CENTER_RIGHT_DT_RESET, 50, CurrentTime + 350);
    ResetCenterDropTargetStatusTime = CurrentTime + 450;
    RPU_PushToTimedSolenoidStack(SOL_TOP_DT_RESET, 50, CurrentTime + 475);
    ResetRightDropTargetStatusTime = CurrentTime + 575;

    if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
      RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 16, CurrentTime + 1000);
    }
    SaucerEjectTime = 0;

    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
  }

  if (WaitForBallToReachOuthole) {
    return MACHINE_STATE_INIT_NEW_BALL;
  }

  // We should only consider the ball initialized when
  // the ball is no longer triggering the SW_OUTHOLE
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

  LastTimeThroughLoop = CurrentTime;
}


void ResetWizardGoals() {
  NumCarryWizardGoals[CurrentPlayer] += (int)(CountBits( WizardGoals[CurrentPlayer] & ~UsedWizardGoals[CurrentPlayer] ));
  WizardGoals[CurrentPlayer] = 0;
  UsedWizardGoals[CurrentPlayer] = 0;
  BasesVisited = 0;
  NeutralZoneHits[CurrentPlayer] = 0;
  TotalSpins[CurrentPlayer] = 0;
  CombosAchieved[CurrentPlayer] = 0; 
  StartScoreAnimation(70000*PlayfieldMultiplier);
  QueueNotification(SOUND_EFFECT_VP_ALL_GOALS_DONE, 10);
}



byte GetNextEnemyVector() {
  unsigned short currentFilledSlots = (SWStatus[CurrentPlayer] & 0x07FF) | BattleLetter;
  if ( currentFilledSlots == 0x07FF ) return 0xFF;

  unsigned long letterPosition = CurrentTime % 11;
  for (byte count = 0; count < 11; count++) {
    if ( (currentFilledSlots & (1 << letterPosition)) == 0 ) {
      break;
    }
    letterPosition = (letterPosition + 1) % 11;
  }

  return letterPosition;
}


byte GameModeStage;
boolean DisplaysNeedRefreshing = false;
unsigned long LastTimePromptPlayed = 0;
unsigned short CurrentBattleLetterPosition = 0xFF;

// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  if (ResetLeftDropTargetStatusTime != 0 && CurrentTime > ResetLeftDropTargetStatusTime) {
    LeftDropTargetStatus = 0;
    ResetLeftDropTargetStatusTime = 0;
  }
  if (ResetCenterDropTargetStatusTime != 0 && CurrentTime > ResetCenterDropTargetStatusTime) {
    CenterDropTargetStatus = 0;
    ResetCenterDropTargetStatusTime = 0;
  }
  if (ResetRightDropTargetStatusTime != 0 && CurrentTime > ResetRightDropTargetStatusTime) {
    RightDropTargetStatus = 0;
    ResetRightDropTargetStatusTime = 0;
  }

  byte letterCount;
  byte goalCount;
  boolean specialAnimationRunning = false;
  unsigned short shortBitMask;

  if ((CurrentTime - LastSwitchHitTime) > 3000) TimersPaused = true;
  else TimersPaused = false;

  if ( (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS) == 0 && TotalSpins[CurrentPlayer] >= SpinnerMaxGoal) {
    WizardGoals[CurrentPlayer] |= WIZARD_GOAL_SPINS;
    QueueNotification(SOUND_EFFECT_VP_SPINNER_GOAL_REACHED, 2);
  }

  if ( (WizardGoals[CurrentPlayer]&WIZARD_GOAL_POP_BASES) == 0 && BasesVisited == BASES_ALL_VISITED) {
    WizardGoals[CurrentPlayer] |= WIZARD_GOAL_POP_BASES;
    QueueNotification(SOUND_EFFECT_VP_BASES_GOAL_REACHED, 2);
  }

  // Check to see if we should be in wizard mode
  goalCount = (int)(CountBits(WizardGoals[CurrentPlayer] & ~UsedWizardGoals[CurrentPlayer])) + NumCarryWizardGoals[CurrentPlayer];
  if (goalCount>0 && GoalsUntilWizard && ((GameMode&GAME_BASE_MODE)==GAME_MODE_UNSTRUCTURED_PLAY || (GameMode&GAME_BASE_MODE)==GAME_MODE_BATTLE || (GameMode&GAME_BASE_MODE)==GAME_MODE_INVASION) ) {
    if ( (goalCount%GoalsUntilWizard)==0 ) {
      SetGameMode(GAME_MODE_WIZARD_START);
      if ((GameMode & GAME_BASE_MODE)!=GAME_MODE_UNSTRUCTURED_PLAY) StartScoreAnimation(100000 * PlayfieldMultiplier);
    }
  }

  switch ( (GameMode & GAME_BASE_MODE) ) {
    case GAME_MODE_SKILL_SHOT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
//        PlaySoundEffect(SOUND_EFFECT_WAITING_FOR_SKILL);
        LastTimePromptPlayed = CurrentTime;
      }

      if (BallFirstSwitchHitTime != 0) {
        if (DEBUG_MESSAGES) {
          Serial.write("Skill shot over\n");
        }
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }

      if (CurrentTime > (LastTimePromptPlayed + 8000)) {
        LastTimePromptPlayed = CurrentTime;
#ifdef RPU_TYPE_1_SOUND
        //        PlaySoundEffect(SOUND_EFFECT_WAITING_FOR_SKILL);
#endif
        QueueNotification(SOUND_EFFECT_VP_LAUNCH_PROMPT_1_1 + ((CurrentTime % 3) * 4) + CurrentPlayer, 1);
      }

      break;


    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        BattleAward = 0;
        BattleLetter = 0;
        DisplaysNeedRefreshing = true;
        TicksCountedTowardsInvasion = 0;
        TicksCountedTowardsStatus = 0;
        InvasionPosition = INVASION_POSITION_NONE;
        IdleMode = IDLE_MODE_NONE;
        if (DEBUG_MESSAGES) {
          Serial.write("Entering unstructured play\n");
        }

        // Reset expiration of shield
        unsigned short bitMask = 0x0001;
        for (byte count = 0; count < 11; count++) {
          SWLettersExpirationTime[count] = 0;
          if (SWLettersLevel[CurrentPlayer] > 0) {
            unsigned long expirationTime = SW_LETTERS_EXPIRATION_BASE / ((unsigned long)SWLettersLevel[CurrentPlayer]);
            if (count < 7) {
              if ((SWStatus[CurrentPlayer] & 0x007F) != 0x007F) {
                if (SWStatus[CurrentPlayer]&bitMask) SWLettersExpirationTime[count] = CurrentTime + expirationTime;
              }
            } else {
              if ((SWStatus[CurrentPlayer] & 0x0780) != 0x0780) {
                if (SWStatus[CurrentPlayer]&bitMask) SWLettersExpirationTime[count] = CurrentTime + expirationTime;
              }
            }
          }
          bitMask *= 2;
        }
      }

      // Check to see if we should reset WizardGoals
      if (WizardGoals[CurrentPlayer]==0x7F) {
        ResetWizardGoals();
      }

      // Check letters for expiration
      shortBitMask = 0x0001;
      for (letterCount = 0; letterCount < 11; letterCount++) {
        if (SWLettersExpirationTime[letterCount] && CurrentTime > SWLettersExpirationTime[letterCount]) {
          SWStatus[CurrentPlayer] &= ~(shortBitMask);
          SWLettersAnimationStartTime[letterCount] = 0;
          SWLettersExpirationTime[letterCount] = 0;
        }
        shortBitMask *= 2;
      }

      // An invasion will start after the top lanes are cleared
      // for 15 seconds of play
      if ((TopLaneStatus[CurrentPlayer] & 0x0F) == 0x00) {
        if (!TimersPaused) {
          TicksCountedTowardsInvasion += (CurrentTime - LastTimeThroughLoop);
        }

        if (TicksCountedTowardsInvasion > INVASION_START_WAIT_TIME) {
          SetGameMode(GAME_MODE_INVASION_START);
        }

      } else {
        TicksCountedTowardsInvasion = 0;
      }

      if (TimersPaused && IdleModeOn) {
        TicksCountedTowardsStatus += (CurrentTime - LastTimeThroughLoop);

        if (TicksCountedTowardsStatus > 68000) {
          IdleMode = IDLE_MODE_NONE;
          TicksCountedTowardsStatus = 0;
        } else if (TicksCountedTowardsStatus > 59000) {
          if (IdleMode != IDLE_MODE_BALL_SEARCH) {
            BallSearchSolenoidToTry = 0;
            BallSearchNextSolenoidTime = CurrentTime - 1;
          }
          if (CurrentTime > BallSearchNextSolenoidTime) {
            // Fire off a solenoid
            BallSearchSolenoidFireTime[BallSearchSolenoidToTry] = CurrentTime;
            RPU_PushToSolenoidStack(BallSearchSols[BallSearchSolenoidToTry], 10);
            BallSearchSolenoidToTry += 1;
            if (BallSearchSolenoidToTry >= NUM_BALL_SEARCH_SOLENOIDS) BallSearchSolenoidToTry = 0;
            BallSearchNextSolenoidTime = CurrentTime + 500;
          }
          IdleMode = IDLE_MODE_BALL_SEARCH;
        } else if (TicksCountedTowardsStatus > 52000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SHIELD) {
            TicksCountedTowardsStatus = 59001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_SHIELD) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_SHIELD, 1);
            IdleMode = IDLE_MODE_ADVERTISE_SHIELD;
          }
        } else if (TicksCountedTowardsStatus > 45000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS) {
            TicksCountedTowardsStatus = 52001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_SPINS) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_SPINS, 1);
            IdleMode = IDLE_MODE_ADVERTISE_SPINS;
          }
        } else if (TicksCountedTowardsStatus > 38000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_7_NZ) {
            TicksCountedTowardsStatus = 45001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_NZS) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_NZS, 1);
            IdleMode = IDLE_MODE_ADVERTISE_NZS;
            ShowLampAnimation(0, 40, CurrentTime, 11, false, false);
            specialAnimationRunning = true;
          }
        } else if (TicksCountedTowardsStatus > 31000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_POP_BASES) {
            TicksCountedTowardsStatus = 38001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_BASES) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_BASES, 1);
            IdleMode = IDLE_MODE_ADVERTISE_BASES;
          }
        } else if (TicksCountedTowardsStatus > 24000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_COMBOS) {
            TicksCountedTowardsStatus = 31001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_COMBOS) {
              byte countBits = CountBits(CombosAchieved[CurrentPlayer]);
              if (countBits==0) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_COMBOS, 1);
              else if (countBits>0) QueueNotification(SOUND_EFFECT_VP_FIVE_COMBOS_LEFT+(countBits-1), 1);
            }
            IdleMode = IDLE_MODE_ADVERTISE_COMBOS;
          }
        } else if (TicksCountedTowardsStatus > 17000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_INVASION) {
            TicksCountedTowardsStatus = 24001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_INVASION) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_INVASION, 1);
            IdleMode = IDLE_MODE_ADVERTISE_INVASION;
          }
        } else if (TicksCountedTowardsStatus > 10000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_BATTLE) {
            TicksCountedTowardsStatus = 17001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_BATTLE) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_BATTLE, 1);
            IdleMode = IDLE_MODE_ADVERTISE_BATTLE;
          }
        } else if (TicksCountedTowardsStatus > 7000) {
          int goalCount = (int)(CountBits((WizardGoals[CurrentPlayer] & ~UsedWizardGoals[CurrentPlayer]))) + NumCarryWizardGoals[CurrentPlayer];
          if (GoalsUntilWizard==0) {
            TicksCountedTowardsStatus = 10001;
          } else {
            byte goalsRemaining = GoalsUntilWizard-(goalCount%GoalsUntilWizard);
            if (goalCount<0) goalsRemaining = (byte)(-1*goalCount);
            
            if (IdleMode != IDLE_MODE_ANNOUNCE_GOALS) {
              QueueNotification(SOUND_EFFECT_VP_ONE_GOAL_FOR_ENEMY-(goalsRemaining-1), 1);
              if (DEBUG_MESSAGES) {
                char buf[256]; 
                sprintf(buf, "Goals remaining = %d, Goals Until Wiz = %d, goalcount = %d, LO=%d, WizG=0x%04X\n", goalsRemaining, GoalsUntilWizard, goalCount, WizardGoals[CurrentPlayer], NumCarryWizardGoals[CurrentPlayer]);
                Serial.write(buf);
              }
            }
            IdleMode = IDLE_MODE_ANNOUNCE_GOALS;
            ShowLampAnimation(2, 40, CurrentTime, 11, false, false);
            specialAnimationRunning = true;
          }
        }
      } else {
        TicksCountedTowardsStatus = 0;
        IdleMode = IDLE_MODE_NONE;
      }


      // Playfield X value is only reset during unstructured play
      if (PlayfieldMultiplierExpiration) {
        if (CurrentTime > PlayfieldMultiplierExpiration) {
          PlayfieldMultiplierExpiration = 0;
          if (PlayfieldMultiplier > 1) QueueNotification(SOUND_EFFECT_VP_1X_PLAYFIELD, 1);
          PlayfieldMultiplier = 1;
        } else {
          for (byte count = 0; count < 4; count++) {
            if (count != CurrentPlayer) OverrideScoreDisplay(count, PlayfieldMultiplier, true);
          }
          DisplaysNeedRefreshing = true;
        }
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        ShowPlayerScores(0xFF, false, false);
      }

      if (SpinnerFrenzyEndTime && CurrentTime > SpinnerFrenzyEndTime) {
        SpinnerFrenzyEndTime = 0;
      }

      break;
    case GAME_MODE_BATTLE_START:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 7000;
        PlaySoundEffect(SOUND_EFFECT_BATTLE_START);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 3000);
        unsigned short letterPosition = GetNextEnemyVector();
        if (letterPosition == 0xFF) {
          BattleLetter = 0x07FF;
          CurrentBattleLetterPosition = 0xFF;
        } else {
          BattleLetter = (1 << letterPosition);
          CurrentBattleLetterPosition = letterPosition;
        }
        BattleAward += 25000;
        RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 7000, true);
        SaucerEjectTime = CurrentTime + 7000;
        StopBackgroundSong();
        LastTimePromptPlayed = 0;

        // Reset Drop Targets
        RPU_PushToTimedSolenoidStack(SOL_LEFT_DT_RESET, 50, CurrentTime + 100);
        ResetLeftDropTargetStatusTime = CurrentTime + 200;
        RPU_PushToTimedSolenoidStack(SOL_CENTER_LEFT_DT_RESET, 50, CurrentTime + 225);
        RPU_PushToTimedSolenoidStack(SOL_CENTER_RIGHT_DT_RESET, 50, CurrentTime + 350);
        ResetCenterDropTargetStatusTime = CurrentTime + 450;
        RPU_PushToTimedSolenoidStack(SOL_TOP_DT_RESET, 50, CurrentTime + 475);
        ResetRightDropTargetStatusTime = CurrentTime + 575;
      }

      if (LastTimePromptPlayed == 0 && CurrentTime > (GameModeStartTime + 4000)) {
        LastTimePromptPlayed = CurrentTime;
        if (CurrentBattleLetterPosition!=0xFF) {
          QueueNotification(SOUND_EFFECT_VP_INCOMING_ENEMY_VECTOR_S1 + CurrentBattleLetterPosition, 6);
        } else {
          QueueNotification(SOUND_EFFECT_VP_SMART_BOMB, 6);
        }
      }

      if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_BATTLE);
      }

      break;

    case GAME_MODE_BATTLE_ADD_ENEMY:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 2000;
        PlaySoundEffect(SOUND_EFFECT_BATTLE_ADD_ENEMY);
        unsigned short letterPosition = GetNextEnemyVector();
        if (letterPosition == 0xFF) {
          // There are no free vectors - pay off jackpot
          StartScoreAnimation(25000 * PlayfieldMultiplier);
        } else {
          QueueNotification(SOUND_EFFECT_VP_INCOMING_ENEMY_VECTOR_S1 + letterPosition, 7);
          BattleLetter |= (1 << letterPosition);
        }

        BattleAward += 10000;
        RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 2000, true);
        SaucerEjectTime = CurrentTime + 2000;
      }

      //      specialAnimationRunning = true;
      //      ShowLampAnimation(0, 80, CurrentTime, 1, false, false);

      if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_BATTLE);
      }
      break;

    case GAME_MODE_BATTLE:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        if (BattleLetter != 0x07FF) {
//          GameModeEndTime = CurrentTime + 15000 + 5000 * ((unsigned long)CountBits(BattleLetter));
          byte numBattles = CountBits(BattleLetter);
          if (numBattles==1) GameModeEndTime = CurrentTime + 20000;
          if (numBattles==2) GameModeEndTime = CurrentTime + 120000;
          if (numBattles>=3) GameModeEndTime = CurrentTime + 240000;
        } else {
          GameModeEndTime = CurrentTime + 30000;
        }
        PlayBackgroundSong(SOUND_EFFECT_BATTLE_SONG_1 + ((CurrentTime / 10) % NUM_BATTLE_SONGS));
      }

      for (byte count = 0; count < 4; count++) {
        if (count != CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, false);
      }

      if (CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_BATTLE_LOST);
      }

      break;

    case GAME_MODE_BATTLE_WON:
      if (GameModeStartTime == 0) {
        WizardGoals[CurrentPlayer] |= WIZARD_GOAL_BATTLE;
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 1500;
        PlaySoundEffect(SOUND_EFFECT_BATTLE_WON);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 3100);
        StartScoreAnimation(5000 * PlayfieldMultiplier);
        BattleAward = 0;
        BattleLetter = 0;
/*
        if (SWStatus[CurrentPlayer] == 0x07FF) {
          // The shield was complete, so we increase SWLetterLevel and clear it
          SWLettersLevel[CurrentPlayer] += 1;
          SWStatus[CurrentPlayer] = 0x0000;
          for (byte count = 0; count < 11; count++) {
            SWLettersAnimationStartTime[count] = CurrentTime;
            SWLettersExpirationTime[count] = 0;
          }
        } else if ( (SWStatus[CurrentPlayer] & 0x007F) == 0x007F) {
          // Clear SR
          SWStatus[CurrentPlayer] &= ~(0x007F);
          for (byte count = 0; count < 7; count++) {
            SWLettersAnimationStartTime[count] = CurrentTime;
            SWLettersExpirationTime[count] = 0;
          }
        } else if ( (SWStatus[CurrentPlayer] & 0x0780) == 0x0780) {
          // Clear WS
          SWStatus[CurrentPlayer] &= ~(0x0780);
          for (byte count = 7; count < 11; count++) {
            SWLettersAnimationStartTime[count] = CurrentTime;
            SWLettersExpirationTime[count] = 0;
          }
        }
*/        

        IncreaseBonusX();
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 40, CurrentTime, 14, false, false);

      if (CurrentTime > GameModeEndTime) {
        StopBackgroundSong();
        QueueNotification(SOUND_EFFECT_VP_BATTLE_WON, 5);
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }

      break;
    case GAME_MODE_BATTLE_LOST:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 1500;
        PlaySoundEffect(SOUND_EFFECT_BATTLE_LOST);
        BattleAward = 0;
        BattleLetter = 0;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(2, 50, CurrentTime, 2, false, true);

      if (CurrentTime > GameModeEndTime) {
        StopBackgroundSong();
        QueueNotification(SOUND_EFFECT_VP_BATTLE_LOST, 5);
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }

      break;

    case GAME_MODE_INVASION_START:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 5000;
        PlaySoundEffect(SOUND_EFFECT_ENEMY_INVASION_ALARM);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 0);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 250);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 500);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 750);
        GameModeStage = 0;
        InvasionPosition = INVASION_POSITION_NONE;
        PlayBackgroundSong(SOUND_EFFECT_BATTLE_SONG_1 + ((CurrentTime / 10) % NUM_BATTLE_SONGS));
        InvasionFlashLevel = 150;
      }

      if (GameModeStage == 0 && CurrentTime > (GameModeStartTime + 1000)) {
        QueueNotification(SOUND_EFFECT_VP_INTERCEPTING_ENEMY_TRANSMISSION, 7);
        GameModeStage = 1;
      } else if (GameModeStage == 1 && CurrentTime > (GameModeStartTime + 2500)) {
        byte invasionNumBitsToShift = CurrentTime % 4;
        InvasionPosition = 1 << invasionNumBitsToShift;
        QueueNotification(SOUND_EFFECT_VP_HUMAN_POSITION_1 + invasionNumBitsToShift, 8);
        GameModeStage = 2;
      }

      if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_INVASION);
      }
      break;

    case GAME_MODE_INVASION:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 70000;
        PlaySoundEffect(SOUND_EFFECT_ENEMY_INVASION_ALARM);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 0);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 250);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 500);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 750);
        LastTimePromptPlayed = 0;
        GameModeStage = 0;
        LastTimePromptPlayed = CurrentTime;
      }

      if ( CurrentTime > (LastTimePromptPlayed + 10000) ) {
        unsigned short positionBeforeMove = InvasionPosition;
        LastTimePromptPlayed = CurrentTime;
        if (GameModeStage >= 3) {
          if (positionBeforeMove & INVASION_POSITION_TOP_1) InvasionPosition |= INVASION_POSITION_TOP_2;
          if (positionBeforeMove & INVASION_POSITION_TOP_2) InvasionPosition |= (INVASION_POSITION_TOP_1 | INVASION_POSITION_TOP_3);
          if (positionBeforeMove & INVASION_POSITION_TOP_3) InvasionPosition |= (INVASION_POSITION_TOP_2 | INVASION_POSITION_TOP_4);
          if (positionBeforeMove & INVASION_POSITION_TOP_4) InvasionPosition |= INVASION_POSITION_TOP_3;
          if (positionBeforeMove & INVASION_POSITION_BULLSEYE || positionBeforeMove & INVASION_POSITION_CAPTIVE) InvasionPosition |= INVASION_POSITION_LOWER_POPS;
        }
        if (GameModeStage == 2) {
          if (positionBeforeMove & INVASION_POSITION_TL_POP) InvasionPosition |= INVASION_POSITION_TR_POP;
          if (positionBeforeMove & INVASION_POSITION_TR_POP) InvasionPosition |= INVASION_POSITION_TL_POP;
        }
        if (GameModeStage >= 1) {
          if (InvasionPosition & INVASION_POSITION_TL_POP) InvasionPosition |= (INVASION_POSITION_MIDDLE_POP | INVASION_POSITION_CAPTIVE);
          if (InvasionPosition & INVASION_POSITION_TR_POP) InvasionPosition |= (INVASION_POSITION_MIDDLE_POP | INVASION_POSITION_BULLSEYE);
        }
        if (InvasionPosition & INVASION_POSITION_TOP_1 || InvasionPosition & INVASION_POSITION_TOP_2) InvasionPosition |= INVASION_POSITION_TL_POP;
        if (InvasionPosition & INVASION_POSITION_TOP_3 || InvasionPosition & INVASION_POSITION_TOP_4) InvasionPosition |= INVASION_POSITION_TR_POP;

        if (GameModeStage == 0) QueueNotification(SOUND_EFFECT_VP_INTRUDER_ALERT, 3);
        else if (InvasionPosition != positionBeforeMove) QueueNotification(SOUND_EFFECT_VP_ENEMY_ADVANCING, 3);

        GameModeStage += 1;
        InvasionFlashLevel = 150 - GameModeStage * 10;
      }

      if (InvasionPosition == INVASION_POSITION_NONE) {
        StartScoreAnimation( PlayfieldMultiplier * (15000 - ((unsigned long)GameModeStage * 2000)) );
        SetGameMode(GAME_MODE_INVASION_WON);
      } else if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_INVASION_LOST);
      }

      break;

    case GAME_MODE_INVASION_WON:
      if (GameModeStartTime == 0) {
        WizardGoals[CurrentPlayer] |= WIZARD_GOAL_INVASION;
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 5000;
        GameModeStage = 0;
        QueueNotification(SOUND_EFFECT_VP_ENEMY_INVASION_DEFEATED, 8);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 40, CurrentTime, 14, false, false);

      if (CurrentTime > GameModeEndTime) {
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        IncreaseBonusX();
      }
      break;

    case GAME_MODE_INVASION_LOST:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3500;
        GameModeStage = 0;
        QueueNotification(SOUND_EFFECT_VP_ENEMY_DESTROYED_SHIELD, 8);
        SWStatus[CurrentPlayer] = 0;
        ShieldDestroyedAnimationStart = CurrentTime;
        PlaySoundEffect(SOUND_EFFECT_SHIELD_DESTROYED);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 500);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 1000);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 1500);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 2000);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 2500);
        RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 3000);
      }

      if (CurrentTime > GameModeEndTime) {
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      break;

    case GAME_MODE_WIZARD_START:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_MODE_QUALIFY_TICKS;
        TimeInSaucer = 0;
        PlayBackgroundSong(SOUND_EFFECT_BATTLE_SONG_1 + ((CurrentTime / 10) % NUM_BATTLE_SONGS));
        QueueNotification(SOUND_EFFECT_VP_SAUCER_TO_ORBIT, 10);
        if (DEBUG_MESSAGES) {
          Serial.write("Playing saucer-to-orbit notification\n\r");
        }
      }

      if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
        if (TimeInSaucer!=0 && CurrentTime>(TimeInSaucer+2000)) {
          ShowPlayerScores(0xFF, false, false);
          SetGameMode(GAME_MODE_WIZARD_WAIT_FOR_BALL);
          RPU_DisableSolenoidStack();
          RPU_SetDisableFlippers(true);
          RPU_TurnOffAllLamps();
          StopBackgroundSong();
//          StopAudio();
          RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 5000, true);
          if (DEBUG_MESSAGES) {
            Serial.write("Waiting for ball to return before starting wizard\n");
          }
        } else if (TimeInSaucer==0) {
          TimeInSaucer = CurrentTime;
          PlaySoundEffect(SOUND_EFFECT_WIZARD_START_SAUCER);
        }
      } else {

        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, false);
        }
        
        if (TimeInSaucer) {
          ShowPlayerScores(0xFF, false, false);
          StopSoundEffect(SOUND_EFFECT_WIZARD_START_SAUCER);
        }
        TimeInSaucer = 0;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(3, 30, CurrentTime, 2, false, true);
      ShowShootAgainLamps();
      ShowSaucerLamps();
      
      if (GameModeEndTime && CurrentTime>GameModeEndTime && !RPU_ReadSingleSwitchState(SW_SAUCER)) {
        NumCarryWizardGoals[CurrentPlayer] -= 1;
        QueueNotification(SOUND_EFFECT_VP_ORBIT_ABANDONED, 10);
        if (DEBUG_MESSAGES) {
          Serial.write("Timed out waiting for wizard start\n");
        }
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      break;

    case GAME_MODE_WIZARD_WAIT_FOR_BALL:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        BallSaveEndTime = CurrentTime + 20000;
        BallSaveUsed = false;
        QueueNotification(SOUND_EFFECT_VP_PREPARING_MISSLES, 9);
      }

      specialAnimationRunning = true;
      RPU_SetLampState(LAMP_SHOOT_AGAIN, 1);
      RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, 1);
      ShowLampAnimation((CurrentTime/300)%4, 30, CurrentTime, 2, false, true);
      //ShowShootAgainLamps();
      
      if (BallSaveUsed) {
        StopSoundEffect(SOUND_EFFECT_WIZARD_START_SAUCER);
        RPU_EnableSolenoidStack();
        RPU_SetDisableFlippers(false);
        SetGameMode(GAME_MODE_WIZARD);
        if (DEBUG_MESSAGES) {
          Serial.write("Ball returned - starting wizard\n");
        }
      }
      break;

    case GAME_MODE_WIZARD:
      if (GameModeStartTime == 0) {
        UsedWizardGoals[CurrentPlayer] |= WizardGoals[CurrentPlayer];
        NumCarryWizardGoals[CurrentPlayer] = 0;
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + ((unsigned long)WizardModeTime * 1000);
        if (WizardModeTime==20) QueueNotification(SOUND_EFFECT_VP_WIZARD_20_SECONDS, 10);
        else if (WizardModeTime==30) QueueNotification(SOUND_EFFECT_VP_WIZARD_30_SECONDS, 10);
        else if (WizardModeTime==45) QueueNotification(SOUND_EFFECT_VP_WIZARD_45_SECONDS, 10);
        else if (WizardModeTime==60) QueueNotification(SOUND_EFFECT_VP_WIZARD_60_SECONDS, 10);
        GameModeStage = 0;
        WizardBonus = 0;
        LastWizardBonus = 0;
        LastTimeWizardBonusShown = 0;
        LastTimePromptPlayed = 0;
        DisplaysNeedRefreshing = false;
      }

      if (BallSaveUsed) {
        BallSaveEndTime = GameModeStartTime + ((unsigned long)WizardModeTime * 1000) + 2000;
        BallSaveUsed = false;
      }

      if (WizardBonus!=LastWizardBonus) {
        LastWizardBonus = WizardBonus;
        LastTimeWizardBonusShown = CurrentTime;
      }

      if (LastTimeWizardBonusShown && CurrentTime>(LastTimeWizardBonusShown+5000)) {
        LastTimeWizardBonusShown = 0;
      }

      if (LastTimeWizardBonusShown) {
        OverrideScoreDisplay(CurrentPlayer, WizardBonus, false);
        DisplaysNeedRefreshing = true;
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        ShowPlayerScores(CurrentPlayer, false, false);
      }

      for (byte count = 0; count < 4; count++) {
        if (count != CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, false);
      }

      if (GameModeStage==0 && CurrentTime>(GameModeStartTime + 4000)) {
        GameModeStage = 1;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_START);
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_WIZARD);
      } else if (CurrentTime>(GameModeEndTime-12000) && GameModeStage==1) {
        QueueNotification(SOUND_EFFECT_VP_WIZARD_10_SECONDS_LEFT, 8);
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_WIZARD_LAST_10);
        GameModeStage = 2;
        LastTimePromptPlayed = CurrentTime;
      } else if (GameModeStage>=1 && CurrentTime > (LastTimePromptPlayed+1000)) {
        LastTimePromptPlayed = CurrentTime;
        RPU_PushToSolenoidStack(SOL_FLASHER_LAMPS, 50, 1);  
      }
      
      
      if (CurrentTime<(GameModeEndTime-5000)) {
        specialAnimationRunning = true;
        ShowLampAnimation(0, 40, CurrentTime, 14, false, false);
      }

      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD_FINISHED_10);
      }
      
      break;

    case GAME_MODE_WIZARD_FINISHED_100:
      if (GameModeStartTime == 0) {
        RPU_DisableSolenoidStack();
        RPU_SetDisableFlippers(true);
        RPU_TurnOffAllLamps();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_FINAL_SHOT_1);
        QueueNotification(SOUND_EFFECT_VP_WIZARD_100_PERCENT_HIT, 8);
        if (WizardBonus) {
          StartScoreAnimation(PlayfieldMultiplier * WizardBonus);
        }
        StopBackgroundSong();
        ShowPlayerScores(0xFF, false, false);
      }

      specialAnimationRunning = true;
      ShowShootAgainLamps();
      ShowLampAnimation(1, 40, CurrentTime-GameModeStartTime, 14, false, false);

      if (CurrentTime>GameModeEndTime) {
        RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 100, true);
        SaucerEjectTime = CurrentTime + 100;
        SetGameMode(GAME_MODE_WIZARD_END_BALL_COLLECT);
      }

      break;
      
    case GAME_MODE_WIZARD_FINISHED_50:
      if (GameModeStartTime == 0) {
        RPU_DisableSolenoidStack();
        RPU_SetDisableFlippers(true);
        RPU_TurnOffAllLamps();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_FINAL_SHOT_2);
        QueueNotification(SOUND_EFFECT_VP_WIZARD_50_PERCENT_HIT, 8);
        if (WizardBonus) {
          unsigned long realizedBonus = 10 * (WizardBonus/20);
          StartScoreAnimation(PlayfieldMultiplier * realizedBonus);
        }
        StopBackgroundSong();
        ShowPlayerScores(0xFF, false, false);
      }

      specialAnimationRunning = true;
      ShowShootAgainLamps();
      ShowLampAnimation(2, 40, CurrentTime-GameModeStartTime, 14, false, false);

      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD_END_BALL_COLLECT);
      }

      break;
    case GAME_MODE_WIZARD_FINISHED_10:
      if (GameModeStartTime == 0) {
        RPU_DisableSolenoidStack();
        RPU_SetDisableFlippers(true);
        RPU_TurnOffAllLamps();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        QueueNotification(SOUND_EFFECT_VP_WIZARD_10_PERCENT_HIT, 8);
        if (WizardBonus) {
          unsigned long realizedBonus = 10 * (WizardBonus/100);
          StartScoreAnimation(PlayfieldMultiplier * realizedBonus);
        }
        StopBackgroundSong();
        ShowPlayerScores(0xFF, false, false);
      }

      specialAnimationRunning = true;
      ShowShootAgainLamps();
      ShowLampAnimation(2, 40, CurrentTime-GameModeStartTime, 2, false, true);

      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD_END_BALL_COLLECT);
      }

      break;

    case GAME_MODE_WIZARD_END_BALL_COLLECT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        BallSaveEndTime = CurrentTime + 20000;
        BallSaveUsed = false;
        QueueNotification(SOUND_EFFECT_VP_WIZARD_END_COLLECT, 8);
        LastTimePromptPlayed = CurrentTime;
      }

      if (CurrentTime > (LastTimePromptPlayed+1000)) {
        LastTimePromptPlayed = CurrentTime;
        RPU_PushToSolenoidStack(SOL_FLASHER_LAMPS, 50);  
      }
      
      specialAnimationRunning = true;
      ShowShootAgainLamps();
      
      if (BallSaveUsed) {
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + ((CurrentTime / 10) % NUM_BACKGROUND_SONGS));
        RPU_EnableSolenoidStack();
        RPU_SetDisableFlippers(false);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      
      break;

  }

  if ( !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    ShowBonusLamps();
    ShowBonusXLamps();
    ShowShootAgainLamps();
    ShowLaneRolloverAndTargetLamps();
    ShowSRWSCircleLamps();
    ShowPopBumperLamps();
    ShowTopLaneLamps();
    ShowSaucerLamps();
    ShowSpinnerLamps();
  }


  // Three types of display modes are shown here:
  // 1) score animation
  // 2) fly-bys
  // 3) normal scores
  if (ScoreAdditionAnimationStartTime != 0) {
    // Score animation
    if ((CurrentTime - ScoreAdditionAnimationStartTime) < 2000) {
      byte displayPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 60;
      byte digitsToShow = 1 + displayPhase / 6;
      if (digitsToShow > 6) digitsToShow = 6;
      unsigned long scoreToShow = ScoreAdditionAnimation;
      for (byte count = 0; count < (6 - digitsToShow); count++) {
        scoreToShow = scoreToShow / 10;
      }
      if (scoreToShow == 0 || displayPhase % 2) scoreToShow = DISPLAY_OVERRIDE_BLANK_SCORE;
      byte countdownDisplay = (1 + CurrentPlayer) % 4;

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, scoreToShow, false);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
      }
    } else {
      byte countdownDisplay = (1 + CurrentPlayer) % 4;
      unsigned long remainingScore = 0;
      if ( (CurrentTime - ScoreAdditionAnimationStartTime) < 5000 ) {
        remainingScore = (((CurrentTime - ScoreAdditionAnimationStartTime) - 2000) * ScoreAdditionAnimation) / 3000;
        if ((remainingScore / 1000) != (LastRemainingAnimatedScoreShown / 1000)) {
          LastRemainingAnimatedScoreShown = remainingScore;
          PlaySoundEffect(SOUND_EFFECT_SCORE_TICK);
        }
      } else {
        CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
        remainingScore = 0;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;
      }

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, ScoreAdditionAnimation - remainingScore, false);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
        else OverrideScoreDisplay(count, CurrentScores[CurrentPlayer] + remainingScore, false);
      }
    }
    if (ScoreAdditionAnimationStartTime) ShowPlayerScores(CurrentPlayer, false, false);
    else ShowPlayerScores(0xFF, false, false);    
  } else if (LastSpinnerHit != 0 && TotalSpins[CurrentPlayer]<SpinnerMaxGoal) {
    OverrideScoreDisplay(CurrentPlayer, SpinnerMaxGoal-TotalSpins[CurrentPlayer], false);
    if (CurrentTime>(LastSpinnerHit+3000)) {
      LastSpinnerHit = 0;
      ShowPlayerScores(0xFF, false, false);
    } else {
      ShowPlayerScores(CurrentPlayer, false, false);      
    }
  } else {
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - LastTimeScoreChanged) > 2000)) ? true : false);

    // Show the player up lamp
    if (BallFirstSwitchHitTime == 0) {
      for (byte count = 0; count < 4; count++) {
        RPU_SetLampState(LAMP_HEAD_PLAYER_1_UP + count, (((CurrentTime / 250) % 2) == 0 || CurrentPlayer != count) ? false : true);
        RPU_SetLampState(LAMP_HEAD_1_PLAYER + count, ((count+1)==CurrentNumPlayers) ? true : false);
      }
    } else {
      for (byte count = 0; count < 4; count++) {
        RPU_SetLampState(LAMP_HEAD_PLAYER_1_UP + count, (CurrentPlayer == count) ? true : false);
        RPU_SetLampState(LAMP_HEAD_1_PLAYER + count, ((count+1)==CurrentNumPlayers) ? true : false);
      }
    }
  }

  // Check to see if ball is in the outhole
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 16, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
          ScoreAdditionAnimationStartTime = 0;
          ScoreAdditionAnimation = 0;
          ShowPlayerScores(0xFF, false, false);
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (!BallSaveUsed && CurrentTime<(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
            BallSaveUsed = true;
            if ( (GameMode & GAME_BASE_MODE)!=GAME_MODE_WIZARD_WAIT_FOR_BALL && (GameMode & GAME_BASE_MODE)!=GAME_MODE_WIZARD_END_BALL_COLLECT ) {
              QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);
              RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 16, CurrentTime);
              BallTimeInTrough = 0;
            } else {
              RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 16, CurrentTime, true);
              BallTimeInTrough = 0;
            }
            RPU_SetLampState(LAMP_SHOOT_AGAIN, 0);
            RPU_SetLampState(LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
          } else {
            ShowPlayerScores(0xFF, false, false);
            PlayBackgroundSong(SOUND_EFFECT_NONE);
            StopAudio();

            PlayfieldMultiplier = 1;
            PlayfieldMultiplierExpiration = 0;
            if (CurrentBallInPlay < BallsPerGame) PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  LastTimeThroughLoop = CurrentTime;
  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {

    Bonus[CurrentPlayer] = CurrentBonus;
    CountdownStartTime = CurrentTime;
    ShowBonusXLamps();
    ShowBonusLamps();

    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  unsigned long countdownDelayTime = 100 - (CurrentBonus * 2);

  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (CurrentBonus) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT);
        CurrentScores[CurrentPlayer] += 1000 * ((unsigned long)BonusX[CurrentPlayer]);
      }

      CurrentBonus -= 1;

      ShowBonusLamps();
    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      BonusCountDownEndTime = CurrentTime + 1000;
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 8, CurrentTime, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 8, CurrentTime + 300, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 8, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
    RPU_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      RPU_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      RPU_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        RPU_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime / 200) % 2 ) {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & 0x0F);
      } else {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}

unsigned long ToplaneDebounce[4] = {0, 0, 0, 0};

void HandleTopLaneHit(byte switchHit) {

  if (ToplaneDebounce[switchHit - SW_1_TOPLANE] != 0) {
    // if it has been less than a 1/4 second since the last hit, reject this one
    if (CurrentTime < (ToplaneDebounce[switchHit - SW_1_TOPLANE] + 250)) return;
  }

  byte laneMask = (1 << (switchHit - SW_1_TOPLANE));
  ToplaneDebounce[switchHit - SW_1_TOPLANE] = CurrentTime;

  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    if ( (switchHit - SW_1_TOPLANE) == SkillShotLane ) {
      // Skill shot hit
      StartScoreAnimation(10000 * CurrentBallInPlay);
      IncreasePlayfieldMultiplier(15000);
      IncreaseBonusX();
      AddToBonus(1);
      PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
      for (byte count = 0; count < 4; count++) {
        TopLaneAnimationStartTime[count] = CurrentTime;
      }
    } else {
      PlaySoundEffect(SOUND_EFFECT_TOP_LANE_REPEAT);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    }
  } else if (InvasionPosition & INVASION_POSITION_TOP_LANE_MASK) {
    if (InvasionPosition & laneMask) {
      // Player hit the invasion lane
      InvasionPosition &= ~(laneMask);
      PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
      AddToBonus(1);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    } else {
      PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_MISS);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    }
  } else {

    unsigned long toplaneLevel = (unsigned long)(TopLaneStatus[CurrentPlayer] / 16);

    if ( laneMask & (TopLaneStatus[CurrentPlayer]) ) {
      PlaySoundEffect(SOUND_EFFECT_TOP_LANE_REPEAT);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    } else {
      PlaySoundEffect(SOUND_EFFECT_TOP_LANE_NEW);
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * toplaneLevel * 1000;
      AddToBonus(1);
      TopLaneStatus[CurrentPlayer] |= laneMask;
      TopLaneAnimationStartTime[switchHit - SW_1_TOPLANE] = CurrentTime;
    }

    // Check to see if this finishes the top lanes
    if ((TopLaneStatus[CurrentPlayer] & 0x0F) == 0x0F) {
      if ((TopLaneStatus[CurrentPlayer] / 16) < 0x0F) {
        TopLaneStatus[CurrentPlayer] += 1;
      }
      TopLaneStatus[CurrentPlayer] &= 0xF0;
      PlaySoundEffect(SOUND_EFFECT_TOP_LANE_LEVEL_FINISHED);
      StartScoreAnimation(4000 * PlayfieldMultiplier * toplaneLevel);
      if ( (GameMode & GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY ) IncreasePlayfieldMultiplier(30000);
    }
  }

}


boolean HandleNeutralZoneHit(byte neutralZoneNumber, boolean spotNextZone) {
  if (NeutralZoneHits[CurrentPlayer] == 0x7F) {
    CurrentScores[CurrentPlayer] += 10 * PlayfieldMultiplier;
    PlaySoundEffect(SOUND_EFFECT_NEUTRAL_ZONE_DUPLICATE);
    return false;
  }

  byte bitMask = 1 << neutralZoneNumber;
  boolean thisNeutralZoneDone = (bitMask & NeutralZoneHits[CurrentPlayer]) ? true : false;
  if (thisNeutralZoneDone && !spotNextZone) {
    CurrentScores[CurrentPlayer] += 10 * PlayfieldMultiplier;
    PlaySoundEffect(SOUND_EFFECT_NEUTRAL_ZONE_DUPLICATE);
    return false;
  }

  if (thisNeutralZoneDone) {
    // We need to spot them a neutral zone
    bitMask = 0x01;
    for (byte count = 0; count < 7; count++) {
      if (!(bitMask & NeutralZoneHits[CurrentPlayer]) ) break;
      bitMask *= 2;
    }
  }
  if (bitMask <= 0x80) {
    // Add in the current neutral zone hit
    NeutralZoneHits[CurrentPlayer] |= bitMask;
    CurrentScores[CurrentPlayer] += 1000 * PlayfieldMultiplier;
    if (NeutralZoneHits[CurrentPlayer] != 0x7F) PlaySoundEffect(SOUND_EFFECT_NEUTRAL_ZONE_HIT);
  } else {
    return false;
  }

  if (NeutralZoneHits[CurrentPlayer] == 0x7F) {
    QueueNotification(SOUND_EFFECT_VP_SEVEN_NEUTRAL_ZONES, 5);
    WizardGoals[CurrentPlayer] |= WIZARD_GOAL_7_NZ;
  }

  return true;
}


boolean AwardSWLetter(byte letterIndex) {
  unsigned short letterBit = (0x0001 << ((unsigned short)letterIndex));

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "SWStatus before hit=0x%04X\n\r", SWStatus[CurrentPlayer]);
    Serial.write(buf);
  }

  WizardBonus += 10000;

  unsigned short lastStatus = SWStatus[CurrentPlayer];
  SWStatus[CurrentPlayer] |= letterBit;
  SWLettersAnimationStartTime[letterIndex] = CurrentTime;
  if (SWLettersLevel[CurrentPlayer] == 0) {
    SWLettersExpirationTime[letterIndex] = 0;
  } else {
    if (SWLettersExpirationTime[letterIndex]) SWLettersExpirationTime[letterIndex] += (SW_LETTERS_EXPIRATION_BASE / ((unsigned long)SWLettersLevel[CurrentPlayer]));
    else SWLettersExpirationTime[letterIndex] = CurrentTime + (SW_LETTERS_EXPIRATION_BASE / ((unsigned long)SWLettersLevel[CurrentPlayer]));
  }

  // Only award words once
  boolean wordCleared = false;
  if (lastStatus != SWStatus[CurrentPlayer]) {
    if ( (SWStatus[CurrentPlayer] & 0x07FF) == 0x07FF ) {
      for (byte count = 0; count < 11; count++) {
        SWLettersAnimationStartTime[count] = CurrentTime;
        SWLettersExpirationTime[count] = CurrentTime + SW_LETTERS_SHIELD_COMPLETE_TIME;
      }
      RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 0);
      RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 400);
      RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 800);
      RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 1800);
      RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 2800);
      RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 3800);
      PlaySoundEffect(SOUND_EFFECT_SRWS_FINISHED);
      QueueNotification(SOUND_EFFECT_VP_SHIELD_COMPLETE, 5);
      SWLettersLevel[CurrentPlayer] += 1;
      StartScoreAnimation(SRWS_COMPLETION_BONUS * PlayfieldMultiplier * (unsigned long)SWLettersLevel[CurrentPlayer]);
      wordCleared = true;
      IncreaseBonusX();
      WizardGoals[CurrentPlayer] |= WIZARD_GOAL_SHIELD;
    } else if ( letterIndex < 7 && (SWStatus[CurrentPlayer] & 0x007F) == 0x007F ) {
      for (byte count = 0; count < 7; count++) {
        SWLettersAnimationStartTime[count] = CurrentTime;
        SWLettersExpirationTime[count] = 0;
      }
      PlaySoundEffect(SOUND_EFFECT_SR_FINISHED);
      QueueNotification(SOUND_EFFECT_VP_SR, 5);
      wordCleared = true;
      IncreaseBonusX();
    } else if ( letterIndex > 6 && (SWStatus[CurrentPlayer] & 0x0780) == 0x0780 ) {
      for (byte count = 0; count < 4; count++) {
        SWLettersAnimationStartTime[count + 7] = CurrentTime;
        SWLettersExpirationTime[count + 7] = 0;
      }
      PlaySoundEffect(SOUND_EFFECT_WS_FINISHED);
      QueueNotification(SOUND_EFFECT_VP_WS, 4);
      wordCleared = true;
      IncreaseBonusX();
    }
  }

  if (BattleLetter & letterBit) {
    StartScoreAnimation(BattleAward * PlayfieldMultiplier);
    AddToBonus(10);
    if (BattleLetter == 0x07FF) {
      BattleLetter = 0;
      SetGameMode(GAME_MODE_BATTLE_WON);
    } else {
      BattleLetter &= ~letterBit;
      if (BattleLetter == 0) {
        SetGameMode(GAME_MODE_BATTLE_WON);
      }
    }
    PlaySoundEffect(SOUND_EFFECT_BATTLE_ENEMY_HIT);
    RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 0);
    RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 200);
    RPU_PushToTimedSolenoidStack(SOL_FLASHER_LAMPS, 50, 400);
  } else {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    AddToBonus(1);
    if (!wordCleared) PlaySoundEffect(SOUND_EFFECT_SW_LETTER_AWARDED);
  }

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "SWStatus after hit=0x%04X (index %d)\n\r", SWStatus[CurrentPlayer], letterIndex);
    Serial.write(buf);
  }

  return true;
}

boolean SpotNextSWLetter() {
  for (byte count = 0; count < 11; count++) {
    unsigned short letterBit = (0x0001 << ((unsigned short)count));
    if ( !(letterBit & SWStatus[CurrentPlayer]) ) {
      AwardSWLetter(count);
      break;
    }
  }

  return false;
}


boolean HandleLeftDropTargetHit(byte switchHit) {
  if (ResetLeftDropTargetStatusTime) return false;

  byte targetBits = 0;
  switch (switchHit) {
    case SW_LEFT_DT_1: targetBits = 0x01; break;
    case SW_LEFT_DT_2: targetBits = 0x02; break;
    case SW_LEFT_DT_3: targetBits = 0x04; break;
    case SW_LEFT_DT_ALL:
      targetBits = 0x07 & (~LeftDropTargetStatus);
      break;
  }

  if (RPU_ReadSingleSwitchState(SW_LEFT_DT_ALL)) targetBits = 0x07 & (~LeftDropTargetStatus);

  // Add scoring & rewards
  if (targetBits & 0x01) {
    AwardSWLetter(SW_LETTER_S1_INDEX);
  }

  if (targetBits & 0x02) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    AddToBonus(1);
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
  }

  // Add scoring & rewards
  if (targetBits & 0x04) {
    AwardSWLetter(SW_LETTER_T_INDEX);
  }

  LeftDropTargetStatus |= targetBits;

  if ((LeftDropTargetStatus & 0x07) == 0x07) {
    RPU_PushToTimedSolenoidStack(SOL_LEFT_DT_RESET, 50, CurrentTime + 100);
    ResetLeftDropTargetStatusTime = CurrentTime + 200;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_RESET);
    StartScoreAnimation(3000 * PlayfieldMultiplier);
    if ( (GameMode & GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY ) IncreasePlayfieldMultiplier(15000);
    UpperPopFrenzyFinish = CurrentTime + 10000 + 5000 * ((unsigned long)NumLeftDTClears[CurrentPlayer]);
    NumLeftDTClears[CurrentPlayer] += 1;
  }

  return true;
}

boolean HandleCenterDropTargetHit(byte switchHit) {
  if (ResetCenterDropTargetStatusTime) return false;

  byte targetBits = 0;
  switch (switchHit) {
    case SW_CENTER_DT_1: targetBits = 0x01; break;
    case SW_CENTER_DT_2: targetBits = 0x02; break;
    case SW_CENTER_DT_3: targetBits = 0x04; break;
    case SW_CENTER_DT_4: targetBits = 0x08; break;
    case SW_CENTER_DT_ALL:
      targetBits = 0x07 & (~CenterDropTargetStatus);
      break;
  }

  if (RPU_ReadSingleSwitchState(SW_CENTER_DT_ALL)) targetBits = 0x07 & (~CenterDropTargetStatus);

  // Add scoring & rewards
  if (targetBits & 0x01) {
    AwardSWLetter(SW_LETTER_E_INDEX);
  }

  // Add scoring & rewards
  if (targetBits & 0x02) {
    AwardSWLetter(SW_LETTER_L1_INDEX);
  }

  // Add scoring & rewards
  if (targetBits & 0x04) {
    AwardSWLetter(SW_LETTER_L2_INDEX);
  }

  // Add scoring & rewards
  if (targetBits & 0x08) {
    AwardSWLetter(SW_LETTER_A1_INDEX);
  }


  CenterDropTargetStatus |= targetBits;

  if ((CenterDropTargetStatus & 0x0F) == 0x0F) {
    RPU_PushToTimedSolenoidStack(SOL_CENTER_LEFT_DT_RESET, 50, CurrentTime + 100);
    RPU_PushToTimedSolenoidStack(SOL_CENTER_RIGHT_DT_RESET, 50, CurrentTime + 225);
    ResetCenterDropTargetStatusTime = CurrentTime + 350;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_RESET);
    StartScoreAnimation(4000 * PlayfieldMultiplier);
    if ( (GameMode & GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY ) IncreasePlayfieldMultiplier(15000);

    NumCenterDTClears[CurrentPlayer] += 1;

    if (NumCenterDTClears[CurrentPlayer] == 1) SaucerValue[CurrentPlayer] = SAUCER_VALUE_2K;
    else if (NumCenterDTClears[CurrentPlayer] == 2) SaucerValue[CurrentPlayer] = SAUCER_VALUE_5K;
    else if (NumCenterDTClears[CurrentPlayer] == 3) SaucerValue[CurrentPlayer] = SAUCER_VALUE_10K;
    else if (NumCenterDTClears[CurrentPlayer] >= 4) SaucerValue[CurrentPlayer] = SAUCER_VALUE_EB;
    SaucerScoreAnimationStart = CurrentTime;

    if (NumCenterDTClears[CurrentPlayer] > 4) OutlaneSpecialLit[CurrentPlayer] = true;
  }

  return true;
}


boolean AwardCombo(byte comboNumber) {
  byte incomingComboStatus = CombosAchieved[CurrentPlayer];
  byte comboBit = 1 << comboNumber;
  boolean newCombo = false;

  CombosAchieved[CurrentPlayer] |= comboBit;

  if (CombosAchieved[CurrentPlayer] != incomingComboStatus) {
    byte numCombos = CountBits((unsigned short)CombosAchieved[CurrentPlayer]);
    if (numCombos < 6) {
      QueueNotification(SOUND_EFFECT_VP_COMBO_1 + (numCombos - 1), 2);
      StartScoreAnimation(PlayfieldMultiplier * COMBO_AWARD * ((unsigned long)numCombos));
    } else {
      QueueNotification(SOUND_EFFECT_VP_COMBOS_COMPLETE, 8);
      StartScoreAnimation(PlayfieldMultiplier * COMBOS_COMPLETE_AWARD);
    }

    if (numCombos == 2) HoldoverAwards[CurrentPlayer] |= HOLDOVER_BONUS_X;
    if (numCombos == 4) HoldoverAwards[CurrentPlayer] |= HOLDOVER_BONUS;
    if (numCombos==CombosToFinishGoal) WizardGoals[CurrentPlayer] |= WIZARD_GOAL_COMBOS;
    newCombo = true;
  } else {
    CurrentScores[CurrentPlayer] += 1000;
  }

  LastLeftInlane = 0;
  LastRightInlane = 0;
  return newCombo;
}


boolean HandleRightDropTargetHit(byte switchHit) {
  if (ResetRightDropTargetStatusTime) return false;

  byte targetBits = 0;
  switch (switchHit) {
    case SW_UPPER_DT_1: targetBits = 0x01; break;
    case SW_UPPER_DT_2: targetBits = 0x02; break;
    case SW_UPPER_DT_3: targetBits = 0x04; break;
    case SW_UPPER_DT_ALL:
      targetBits = 0x07 & (~RightDropTargetStatus);
      break;
  }

  if (RPU_ReadSingleSwitchState(SW_UPPER_DT_ALL)) targetBits = 0x07 & (~RightDropTargetStatus);

  if (targetBits & 0x01) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    AddToBonus(1);
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
  }

  // Add scoring & rewards
  if (targetBits & 0x02) {
    AwardSWLetter(SW_LETTER_R1_INDEX);
  }

  if (targetBits & 0x04) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
    AddToBonus(1);
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
  }

  RightDropTargetStatus |= targetBits;

  if ((RightDropTargetStatus & 0x07) == 0x07) {
    RPU_PushToTimedSolenoidStack(SOL_TOP_DT_RESET, 50, CurrentTime + 100);
    ResetRightDropTargetStatusTime = CurrentTime + 200;
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_RESET);
    StartScoreAnimation(3000 * PlayfieldMultiplier);
    if ( (GameMode & GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY ) IncreasePlayfieldMultiplier(15000);
    NumRightDTClears[CurrentPlayer] += 1;

    if (NumRightDTClears[CurrentPlayer] == 1) CaptiveBallLit[CurrentPlayer] = true;
    else if (NumRightDTClears[CurrentPlayer] == 2) LowerPopStatus[CurrentPlayer] += 1;

    if (NumRightDTClears[CurrentPlayer] >= 3) {
      if (SpinnerFrenzyEndTime) SpinnerFrenzyEndTime += 30000;
      else SpinnerFrenzyEndTime = CurrentTime + 30000;
    }

    if (NumRightDTClears[CurrentPlayer] >= 4) {
      BullseyeSpecialLit[CurrentPlayer] = true;
    }

  }

  return true;
}


void RotateWSLetters(boolean cycleRight) {
  unsigned int wsLetters = SWStatus[CurrentPlayer] & SW_STATUS_WS_MASK;
  unsigned int wsBattleLetters = BattleLetter & SW_STATUS_WS_MASK;

  if (cycleRight) {
    wsLetters *= 2;
    if ( wsLetters & 0x0800 ) wsLetters |= 0x0080;
    wsLetters &= SW_STATUS_WS_MASK;

    wsBattleLetters *= 2;
    if ( wsBattleLetters & 0x0800 ) wsBattleLetters |= 0x0080;
    wsBattleLetters &= SW_STATUS_WS_MASK;

    if (CurrentBattleLetterPosition >= SW_LETTER_W_INDEX && CurrentBattleLetterPosition <= SW_LETTER_S2_INDEX) {
      CurrentBattleLetterPosition += 1;
      if (CurrentBattleLetterPosition > SW_LETTER_S2_INDEX) CurrentBattleLetterPosition = SW_LETTER_W_INDEX;
    }

    unsigned long tempAnim = SWLettersAnimationStartTime[SW_LETTER_S2_INDEX];
    for (byte count = SW_LETTER_W_INDEX; count < SW_LETTER_S2_INDEX; count++) {
      SWLettersAnimationStartTime[count + 1] = SWLettersAnimationStartTime[count];
    }
    SWLettersAnimationStartTime[SW_LETTER_W_INDEX] = tempAnim;

  } else {
    wsLetters /= 2;
    if ( wsLetters & 0x0040 ) wsLetters |= 0x0400;
    wsLetters &= SW_STATUS_WS_MASK;

    wsBattleLetters /= 2;
    if ( wsBattleLetters & 0x0040 ) wsBattleLetters |= 0x0400;
    wsBattleLetters &= SW_STATUS_WS_MASK;

    if (CurrentBattleLetterPosition >= SW_LETTER_W_INDEX && CurrentBattleLetterPosition <= SW_LETTER_S2_INDEX) {
      CurrentBattleLetterPosition -= 1;
      if (CurrentBattleLetterPosition < SW_LETTER_W_INDEX) CurrentBattleLetterPosition = SW_LETTER_S2_INDEX;
    }

    unsigned long tempAnim = SWLettersAnimationStartTime[SW_LETTER_W_INDEX];
    for (byte count = SW_LETTER_W_INDEX; count < SW_LETTER_S2_INDEX; count++) {
      SWLettersAnimationStartTime[count] = SWLettersAnimationStartTime[count + 1];
    }
    SWLettersAnimationStartTime[SW_LETTER_S2_INDEX] = tempAnim;

  }

  SWStatus[CurrentPlayer] &= ~SW_STATUS_WS_MASK;
  SWStatus[CurrentPlayer] |= wsLetters;

  BattleLetter &= ~SW_STATUS_WS_MASK;
  BattleLetter |= wsBattleLetters;
}



int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay();
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(0xFF, false, false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);

    if (SamePlayerShootsAgain) {
      //PlaySoundEffect(SOUND_EFFECT_SHOOT_AGAIN);
      QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          RPU_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  byte switchHit;
  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;

  if (NumTiltWarnings <= MaxTiltWarnings) {
    while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Switch Hit = %d\n", switchHit);
        Serial.write(buf);
      }

      switch (switchHit) {
        case SW_SLAM:
          //          RPU_DisableSolenoidStack();
          //          RPU_SetDisableFlippers(true);
          //          RPU_TurnOffAllLamps();
          //          RPU_SetLampState(GAME_OVER, 1);
          //          delay(1000);
          //          return MACHINE_STATE_ATTRACT;
          break;
        case SW_PLUMB_TILT:
        case SW_ROLL_TILT:
        case SW_PLAYFIELD_TILT:
          // This should be debounced
          if (IdleMode != IDLE_MODE_BALL_SEARCH && (CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings > MaxTiltWarnings) {
              RPU_DisableSolenoidStack();
              RPU_SetDisableFlippers(true);
              RPU_TurnOffAllLamps();
              StopAudio();
              PlaySoundEffect(SOUND_EFFECT_TILT);
              RPU_SetLampState(LAMP_HEAD_TILT, 1);
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_BOOT;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_RIGHT_BULLSEYE:
          if (LastLeftInlane && CurrentTime < (LastLeftInlane + COMBO_AVAILABLE_TIME)) {
            AwardCombo(COMBO_LEFT_TO_BULLSEYE);
          }
          
          if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD) {
            SetGameMode(GAME_MODE_WIZARD_FINISHED_50);
          }
          
          if (InvasionPosition & INVASION_POSITION_BULLSEYE) {
            InvasionPosition &= ~(INVASION_POSITION_BULLSEYE);
            CurrentScores[CurrentPlayer] += 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (BullseyeSpecialLit[CurrentPlayer]) {
            StartScoreAnimation(PlayfieldMultiplier * 2000);
            PlaySoundEffect(SOUND_EFFECT_BULLSEYE_LIT);
          } else {
            CurrentScores[CurrentPlayer] += 10;
            PlaySoundEffect(SOUND_EFFECT_BULLSEYE_UNLIT);
          }
          RotateWSLetters(true);
          LastSwitchHitTime = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_DT_STANDUP:
          HandleNeutralZoneHit(NEUTRAL_ZONE_1, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LOWER_TOP_LEFT_SU:
          HandleNeutralZoneHit(NEUTRAL_ZONE_2, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_UPPER_TOP_LEFT_SU:
          HandleNeutralZoneHit(NEUTRAL_ZONE_3, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_MIDDLE_RIGHT_SU:
          HandleNeutralZoneHit(NEUTRAL_ZONE_4, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_UPPER_DT_STANDUP:
          HandleNeutralZoneHit(NEUTRAL_ZONE_5, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_RIGHT_SU:
          HandleNeutralZoneHit(NEUTRAL_ZONE_6, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_CENTER_STANDUP:
          HandleNeutralZoneHit(NEUTRAL_ZONE_7, (GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE);
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_DT_1:
        case SW_LEFT_DT_2:
        case SW_LEFT_DT_3:
        case SW_LEFT_DT_ALL:
          if (HandleLeftDropTargetHit(switchHit)) {
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          }
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_CENTER_DT_1:
        case SW_CENTER_DT_2:
        case SW_CENTER_DT_3:
        case SW_CENTER_DT_4:
        case SW_CENTER_DT_ALL:
          if (HandleCenterDropTargetHit(switchHit)) {
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          }
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_UPPER_DT_1:
        case SW_UPPER_DT_2:
        case SW_UPPER_DT_3:
        case SW_UPPER_DT_ALL:
          if (HandleRightDropTargetHit(switchHit)) {
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          }
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_W_ROLLOVER:
          AwardSWLetter(SW_LETTER_W_INDEX);
          if (LastRightInlane && CurrentTime < (LastRightInlane + COMBO_AVAILABLE_TIME)) {
            AwardCombo(COMBO_RIGHT_TO_LEFT_ALLEY_PASS);
          }
          LastLeftInlane = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_A_ROLLOVER:
          AwardSWLetter(SW_LETTER_A2_INDEX);
          LastLeftInlane = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_R_ROLLOVER:
          AwardSWLetter(SW_LETTER_R2_INDEX);
          LastRightInlane = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_S_ROLLOVER:
          AwardSWLetter(SW_LETTER_S2_INDEX);
          if (LastLeftInlane && CurrentTime < (LastLeftInlane + COMBO_AVAILABLE_TIME)) {
            AwardCombo(COMBO_LEFT_TO_RIGHT_ALLEY_PASS);
          }
          LastRightInlane = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_1_TOPLANE:
        case SW_2_TOPLANE:
        case SW_3_TOPLANE:
        case SW_4_TOPLANE:
          WizardBonus += 5000;
          HandleTopLaneHit(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SLINGSHOT:
        case SW_RIGHT_SLINGSHOT:
          if (CurrentTime < (BallSearchSolenoidFireTime[6] + 150)) break;
          if (CurrentTime < (BallSearchSolenoidFireTime[7] + 150)) break;
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
          PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_CENTER_POP:
          WizardBonus += 100;        
          if (CurrentTime < (BallSearchSolenoidFireTime[4] + 150)) break;
          if (InvasionPosition & INVASION_POSITION_MIDDLE_POP) {
            InvasionPosition &= ~(INVASION_POSITION_MIDDLE_POP);
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (UpperPopFrenzyFinish) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_FRENZY_BUMPER_HIT);
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
          }
          BasesVisited |= BASE_VISIT_TOP_CENTER_POP;
          TopCenterPopLastHit = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_LEFT_POP:
          WizardBonus += 100;        
          if (CurrentTime < (BallSearchSolenoidFireTime[2] + 150)) break;
          if (InvasionPosition & INVASION_POSITION_TL_POP) {
            InvasionPosition &= ~(INVASION_POSITION_TL_POP);
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (UpperPopFrenzyFinish) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_FRENZY_BUMPER_HIT);
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
          }
          BasesVisited |= BASE_VISIT_TOP_LEFT_POP;
          TopLeftPopLastHit = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_RIGHT_POP:
          WizardBonus += 100;        
          if (CurrentTime < (BallSearchSolenoidFireTime[3] + 150)) break;
          if (InvasionPosition & INVASION_POSITION_TR_POP) {
            InvasionPosition &= ~(INVASION_POSITION_TR_POP);
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (UpperPopFrenzyFinish) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_FRENZY_BUMPER_HIT);
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
          }
          BasesVisited |= BASE_VISIT_TOP_RIGHT_POP;
          TopRightPopLastHit = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_BOTTOM_LEFT_POP:
          WizardBonus += 100;        
          if (CurrentTime < (BallSearchSolenoidFireTime[0] + 150)) break;
          if (InvasionPosition & INVASION_POSITION_LOWER_POPS) {
            InvasionPosition &= ~(INVASION_POSITION_LOWER_POPS);
            CurrentScores[CurrentPlayer] += 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (LowerPopStatus[CurrentPlayer]) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_LOWER_BUMPER_HIT);
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
            PlaySoundEffect(SOUND_EFFECT_LOWER_BUMPER_HIT);
          }
          BasesVisited |= BASE_VISIT_BOTTOM_LEFT_POP;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_BOTTOM_RIGHT_POP:
          WizardBonus += 100;        
          if (CurrentTime < (BallSearchSolenoidFireTime[5] + 150)) break;
          if (InvasionPosition & INVASION_POSITION_LOWER_POPS) {
            InvasionPosition &= ~(INVASION_POSITION_LOWER_POPS);
            CurrentScores[CurrentPlayer] += 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (LowerPopStatus[CurrentPlayer]) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            PlaySoundEffect(SOUND_EFFECT_LOWER_BUMPER_HIT);
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
            PlaySoundEffect(SOUND_EFFECT_LOWER_BUMPER_HIT);
          }
          RPU_PushToSolenoidStack(SOL_BOTTOM_RIGHT_POP, 10);
          BasesVisited |= BASE_VISIT_BOTTOM_RIGHT_POP;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SPINNER:
          WizardBonus += 2000;
          if (LastRightInlane && CurrentTime < (LastRightInlane + COMBO_AVAILABLE_TIME)) {
            if (AwardCombo(COMBO_RIGHT_TO_LEFT_SPINNER)) {
              if (SpinnerAccelerators) {
                if (TotalSpins[CurrentPlayer]>(SpinnerMaxGoal-14)) TotalSpins[CurrentPlayer] = SpinnerMaxGoal;
                else TotalSpins[CurrentPlayer] += 14;
              }
            } else {
              if (SpinnerAccelerators) {
                if (TotalSpins[CurrentPlayer]>(SpinnerMaxGoal-24)) TotalSpins[CurrentPlayer] = SpinnerMaxGoal;
                else TotalSpins[CurrentPlayer] += 24;
              }
            }
          }
          if ((GameMode & GAME_BASE_MODE) == GAME_MODE_INVASION && SpinnerAccelerators) {
            TotalSpins[CurrentPlayer] += 3;
          } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE && SpinnerAccelerators) {
            TotalSpins[CurrentPlayer] += 2;
          } else if ((GameMode & GAME_BASE_MODE) != GAME_MODE_SKILL_SHOT) {
            TotalSpins[CurrentPlayer] += 1;
          }
          if (TotalSpins[CurrentPlayer] > SpinnerMaxGoal) TotalSpins[CurrentPlayer] = SpinnerMaxGoal;

          if ((GameMode & GAME_BASE_MODE) != GAME_MODE_SKILL_SHOT) {
            if (SpinnerFrenzyEndTime || (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS)) {
              CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
            } else {
              CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
            }
            PlaySoundEffect(SOUND_EFFECT_LEFT_SPINNER);
          }
          //if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          LastSpinnerHit = CurrentTime;          
          break;
        case SW_RIGHT_SPINNER:
          WizardBonus += 2000;
          if (LastLeftInlane && CurrentTime < (LastLeftInlane + COMBO_AVAILABLE_TIME)) {
            if (AwardCombo(COMBO_LEFT_TO_RIGHT_SPINNER)) {
              if (SpinnerAccelerators) {
                if (TotalSpins[CurrentPlayer]>(SpinnerMaxGoal-19)) TotalSpins[CurrentPlayer] = SpinnerMaxGoal;
                else TotalSpins[CurrentPlayer] += 19;
              }
            } else {
              if (SpinnerAccelerators) {
                if (TotalSpins[CurrentPlayer]>(SpinnerMaxGoal-29)) TotalSpins[CurrentPlayer] = SpinnerMaxGoal;
                else TotalSpins[CurrentPlayer] += 29;
              }
            }
          }
          if ((GameMode & GAME_BASE_MODE) == GAME_MODE_INVASION && SpinnerAccelerators) {
            TotalSpins[CurrentPlayer] += 4;
          } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE && SpinnerAccelerators) {
            TotalSpins[CurrentPlayer] += 3;
          } else {
            TotalSpins[CurrentPlayer] += 1;
          }
          if (TotalSpins[CurrentPlayer] > SpinnerMaxGoal) TotalSpins[CurrentPlayer] = SpinnerMaxGoal;
          if (SpinnerFrenzyEndTime || (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS)) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
          }
          PlaySoundEffect(SOUND_EFFECT_RIGHT_SPINNER);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          LastSpinnerHit = CurrentTime;
          break;
        case SW_CAPTIVE_BALL:
          if (LastRightInlane && CurrentTime < (LastRightInlane + COMBO_AVAILABLE_TIME)) {
            AwardCombo(COMBO_RIGHT_TO_CAPTIVE);
          }
          if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD) {
            SetGameMode(GAME_MODE_WIZARD_FINISHED_50);
          }
          if (InvasionPosition & INVASION_POSITION_CAPTIVE) {
            InvasionPosition &= ~(INVASION_POSITION_CAPTIVE);
            CurrentScores[CurrentPlayer] += 1000;
            PlaySoundEffect(SOUND_EFFECT_INVADING_ENEMY_HIT);
          } else if (CaptiveBallLit[CurrentPlayer]) {
            SpotNextSWLetter();
          } else {
            CurrentScores[CurrentPlayer] += 10;
            PlaySoundEffect(SOUND_EFFECT_CAPTIVE_BALL_UNLIT);
          }
          RotateWSLetters(false);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SPECIAL:
        case SW_RIGHT_SPECIAL:
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2000;
          AddToBonus(1);
          if (OutlaneSpecialLit[CurrentPlayer]) {
            OutlaneSpecialLit[CurrentPlayer] = false;
            AwardSpecial();
            NumCenterDTClears[CurrentPlayer] = 2;
            SaucerValue[CurrentPlayer] = SAUCER_VALUE_5K;
          } else {
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
          }
          if (BallSaveEndTime!=0) {
            BallSaveEndTime += 3000;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_SAUCER:
          if (CurrentTime > SaucerEjectTime) {
            if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
              StartScoreAnimation(50000 * PlayfieldMultiplier);
              SetGameMode(GAME_MODE_BATTLE_START);
              PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
            } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY) {
              SetGameMode(GAME_MODE_BATTLE_START);
            } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_BATTLE) {
              SetGameMode(GAME_MODE_BATTLE_ADD_ENEMY);
            } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD) {
              SetGameMode(GAME_MODE_WIZARD_FINISHED_100);
              SaucerEjectTime = CurrentTime + 500;
            } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_START) {
              TimeInSaucer = 0;
            } else {
              RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 500, true);
              SaucerEjectTime = CurrentTime + 500;
            }
            switch (SaucerValue[CurrentPlayer]) {
              case SAUCER_VALUE_1K: CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000; break;
              case SAUCER_VALUE_2K: CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2000; break;
              case SAUCER_VALUE_5K: CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000; break;
              case SAUCER_VALUE_10K: CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10000; break;
              case SAUCER_VALUE_EB: 
                NumCenterDTClears[CurrentPlayer] = 2;
                SaucerValue[CurrentPlayer] = SAUCER_VALUE_5K;
                AwardExtraBall(); 
                break;
            };
            SaucerScoreAnimationStart = CurrentTime;

          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay < 2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else if (AllowResetAfterBallOne) {
            // If the first ball is over, pressing start again resets the game
            if (Credits >= 1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
                RPU_SetDisplayCredits(Credits, !FreePlayMode);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
          }
          if (DEBUG_MESSAGES) {
            Serial.write("Start game button pressed\n\r");
          }
          break;
      }
    }
  } else {
    // We're tilted, so just wait for outhole
    while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_BOOT;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
      }
    }
  }

  if (lastBallFirstSwitchHitTime==0 && BallFirstSwitchHitTime!=0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)BallSaveNumSeconds)*1000;
  }
  if (CurrentTime>(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > RPU_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= RPU_OS_MAX_DISPLAY_SCORE;
    if (!TournamentScoring) AddSpecialCredit();
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (!ExtraBallCollected) {
            AwardExtraBall();
          }
        }
      }
    }

  }

  return returnState;
}


unsigned long LastLEDUpdateTime = 0;
byte LEDPhase = 0;
unsigned long NumLoops = 0;
unsigned long LastLoopReportTime = 0;

void loop() {

/*
  if (DEBUG_MESSAGES) {
    NumLoops += 1;
    if (CurrentTime>(LastLoopReportTime+1000)) {
      LastLoopReportTime = CurrentTime;
      char buf[128];
      sprintf(buf, "Loop running at %lu Hz\n", NumLoops);
      Serial.write(buf);
      NumLoops = 0;
    }
  }
*/
  
  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_ApplyFlashToLamps(CurrentTime);
  RPU_UpdateTimedSolenoidStack(CurrentTime);
  RPU_UpdateTimedSoundStack(CurrentTime);
  UpdateSoundQueue();
  ServiceNotificationQueue();

  if (LastLEDUpdateTime == 0 || (CurrentTime - LastLEDUpdateTime) > 250) {
    LastLEDUpdateTime = CurrentTime;
    RPU_SetBoardLEDs((LEDPhase % 8) == 1 || (LEDPhase % 8) == 3, (LEDPhase % 8) == 5 || (LEDPhase % 8) == 7);
    LEDPhase += 1;
  }
}
